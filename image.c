#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <cJSON.h>
// gcc -o image image.c $(curl-config --cflags) $(curl-config --libs) -lcjson -I/usr/include/cjson/
// ./image "a white siamese cat"
// 用于存储响应数据的结构体
struct MemoryStruct
{
    char *memory;
    size_t size;
};

// 用于写文件的回调函数
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// 下载图像的函数
void download_image(const char *url, const char *filename)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);

    // 打开文件用于写入
    fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("Error opening file");
        return;
    }

    // 初始化curl
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // 执行下载
        res = curl_easy_perform(curl);

        // 检查错误
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // 清理
        curl_easy_cleanup(curl);
    }
    fclose(fp);
    curl_global_cleanup();
}

// curl的写回调函数，用于保存返回的数据
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, struct MemoryStruct *userp)
{
    size_t realSize = size * nmemb;
    char *ptr = realloc(userp->memory, userp->size + realSize + 1);
    if (ptr == NULL)
    {
        // out of memory
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    userp->memory = ptr;
    memcpy(&(userp->memory[userp->size]), contents, realSize);
    userp->size += realSize;
    userp->memory[userp->size] = 0;

    return realSize;
}

void generate_image(const char *prompt) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    char *postFields;  // API请求的完整字段

    // 初始化chunk结构体
    chunk.memory = malloc(1);  // 初始分配1字节，realloc将调整大小
    chunk.size = 0;    // 无数据

    // 使用cJSON构建请求体
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "model", cJSON_CreateString("dall-e-3"));
    cJSON_AddItemToObject(json, "prompt", cJSON_CreateString(prompt));
    cJSON_AddItemToObject(json, "n", cJSON_CreateNumber(1));
    cJSON_AddItemToObject(json, "size", cJSON_CreateString("1024x1024"));

    postFields = cJSON_PrintUnformatted(json);
    cJSON_Delete(json); // 释放cJSON对象

    curl_global_init(CURL_GLOBAL_ALL);

    // 初始化curl会话
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Authorization: Bearer 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");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/images/generations");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 设置写回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        // 设置写数据
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // 执行请求
        res = curl_easy_perform(curl);

        // 检查错误
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else
        {
            printf("%lu bytes retrieved\n", (unsigned long)chunk.size);
            // 打印或处理响应数据
            printf("Response: %s\n", chunk.memory);
            // 执行请求后的处理
            if (res == CURLE_OK)
            {
                cJSON *json = cJSON_Parse(chunk.memory);
                if (json == NULL)
                {
                    printf("Error before: [%s]\n", cJSON_GetErrorPtr());
                }
                else
                {
                    cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
                    if (cJSON_IsArray(data))
                    {
                        cJSON *first_item = cJSON_GetArrayItem(data, 0);
                        if (first_item != NULL)
                        {
                            cJSON *url = cJSON_GetObjectItemCaseSensitive(first_item, "url");
                            if (cJSON_IsString(url) && (url->valuestring != NULL))
                            {
                                printf("Found URL: %s\n", url->valuestring);
                                download_image(url->valuestring, "downloaded_image.png");
                            }
                        }
                    }
                    cJSON_Delete(json);
                }
            }
        }

        // 清理
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();

    // 释放响应内存
    free(chunk.memory);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <prompt>\n", argv[0]);
        return 1;
    }

    generate_image(argv[1]);
    return 0;
}
