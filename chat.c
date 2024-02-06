#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cJSON.h>
// apt-get install libcurl4-openssl-dev
// apt-get install libcjson-dev
// gcc -o chat chat.c $(curl-config --cflags) $(curl-config --libs) -lcjson -I/usr/include/cjson/
#if 1
#define debug(fmt, args...) printf(fmt, ##args)
#else
#define debug(fmt, args...)
#endif
typedef struct Message
{
    char *role;
    char *content;
    struct Message *next;
} Message;

struct response
{
    char *data;
    size_t size;
};

void add_message(Message **head, const char *role, const char *content)
{
    Message *new_message = malloc(sizeof(Message));
    new_message->role = strdup(role);
    new_message->content = strdup(content);
    new_message->next = NULL;

    if (*head == NULL)
    {
        *head = new_message;
    }
    else
    {
        Message *current = *head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_message;
    }
}

char *create_json_payload(Message *head)
{
    // 创建一个 JSON 对象
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "model", cJSON_CreateString("gpt-4-turbo-preview"));

    // 创建消息数组
    cJSON *messages = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "messages", messages);

    // 遍历消息链表并添加到消息数组
    for (Message *current = head; current != NULL; current = current->next)
    {
        cJSON *message = cJSON_CreateObject();
        cJSON_AddItemToArray(messages, message);
        cJSON_AddStringToObject(message, "role", current->role);
        cJSON_AddStringToObject(message, "content", current->content);
    }

    // 将 JSON 对象转换为格式化的字符串
    char *json = cJSON_Print(root);
    // 将 JSON 对象转换为不格式化的字符串
    // char *json = cJSON_PrintUnformatted(root);

    // 清理 cJSON 对象
    cJSON_Delete(root);
    return json;
}

// 写入响应数据的回调函数
size_t write_response(void *ptr, size_t size, size_t nmemb, struct response *r)
{
    size_t new_length = r->size + size * nmemb;
    r->data = realloc(r->data, new_length + 1);
    if (r->data)
    {
        memcpy(r->data + r->size, ptr, size * nmemb);
        r->data[new_length] = '\0';
        r->size = new_length;
    }
    return size * nmemb;
}

// 发送请求并获取响应
void send_request(const char *json_payload, struct response *resp)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

    resp->data = malloc(1); // 将被 write_response 回调函数增长
    resp->size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Authorization: Bearer apikey");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void free_messages(Message *head)
{
    Message *current = head;
    while (current != NULL)
    {
        Message *next = current->next;
        free(current->role);
        free(current->content);
        free(current);
        current = next;
    }
}

char *extract_ai_response(const char *response)
{
    cJSON *json = cJSON_Parse(response);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return NULL;
    }

    const cJSON *choices = cJSON_GetObjectItemCaseSensitive(json, "choices");
    if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0)
    {
        const cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
        const cJSON *message = cJSON_GetObjectItemCaseSensitive(first_choice, "message");
        const cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");

        char *ai_response = NULL;
        if (cJSON_IsString(content) && (content->valuestring != NULL))
        {
            ai_response = strdup(content->valuestring);
        }

        cJSON_Delete(json);
        return ai_response;
    }

    cJSON_Delete(json);
    return NULL;
}


void process_control_command(cJSON *json) {
    // 获取 "operation"
    cJSON *operation = cJSON_GetObjectItemCaseSensitive(json, "operation");
    if (!cJSON_IsString(operation) || (operation->valuestring == NULL)) {
        printf("Operation missing or incorrect format\n");
        return;
    }

    // 获取 "parameters"
    cJSON *parameters = cJSON_GetObjectItemCaseSensitive(json, "parameters");
    if (!cJSON_IsObject(parameters)) {
        printf("Parameters missing or incorrect format\n");
        return;
    }
    cJSON *status = cJSON_GetObjectItemCaseSensitive(parameters, "status");
    if (!cJSON_IsString(status) || (status->valuestring == NULL)) {
        printf("Status missing or incorrect format\n");
        return;
    }

    // 根据不同的 "operation" 执行不同的逻辑
    if (strcmp(operation->valuestring, "activateFusion") == 0) {
        // 处理 activateFusion 操作
        printf("Activating Fusion, Status: %s\n", status->valuestring);
    } else if (strcmp(operation->valuestring, "switchLight") == 0) {
        // 处理 switchLight 操作
        printf("Switching Light, Status: %s\n", status->valuestring);
    } else {
        // 未知操作
        printf("Unknown operation: %s\n", operation->valuestring);
    }
}

void process_dialog(cJSON *json) {
    // 获取 "message"
    cJSON *message = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cJSON_IsString(message) && (message->valuestring != NULL)) {
        printf("Message: %s\n", message->valuestring);
    } else {
        printf("Message missing or incorrect format\n");
    }
}
long getFileSize(FILE *file) {
    long fileSize = 0;
    fseek(file, 0, SEEK_END); // 移动文件指针到文件末尾
    fileSize = ftell(file);   // 获取当前文件指针的位置，即文件大小
    rewind(file);             // 将文件指针重新定位到文件开头
    return fileSize;
}
int main()
{
    Message *history = NULL;
    struct response resp;
    char user_input[2048]; // 用户输入的缓冲区

    int init = 1;

    while (1)
    {
        if (init == 1)
        {
            // 知识库
            if(1)
            {
                // 读取文件
                FILE *fp = fopen("./dev_ctrl.json", "r");
                if (fp == NULL)
                {
                    fprintf(stderr, "Error opening file\n");
                    return 1;
                }
                // 获取文件大小
                long size = getFileSize(fp);
                printf("File size: %ld bytes\n", size);
                // 为文件内容分配内存
                char *knowledge_base = (char *)malloc(size + 64);
                memset(knowledge_base, 0, sizeof(knowledge_base));
                // 读取文件内容
                fread(knowledge_base, 1, size, fp);
                // 添加用户输入到对话历史
                add_message(&history, "user", knowledge_base);
                free(knowledge_base);
            }
            // 提示词
            if (1)
            {
                // 读取文件
                FILE *fp = fopen("./Prompt.txt", "r");
                if (fp == NULL)
                {
                    fprintf(stderr, "Error opening file\n");
                    return 1;
                }
                // 获取文件大小
                long size = getFileSize(fp);
                printf("File size: %ld bytes\n", size);
                // 为文件内容分配内存
                char *prompt = (char *)malloc(size + 64);
                memset(prompt, 0, sizeof(prompt));
                // 读取文件内容
                fread(prompt, 1, size, fp);
                // 添加用户输入到对话历史
                add_message(&history, "user", prompt);
                free(prompt);
            }
            init = 0;
        }
        else
        {
            printf("You: ");
            memset(user_input, 0, sizeof(user_input));
            if (fgets(user_input, 2048, stdin) == NULL)
            {
                break; // 如果读取失败或遇到 EOF，则退出循环
            }
            user_input[strcspn(user_input, "\n")] = 0; // 去除换行符
            // 添加用户输入到对话历史
            add_message(&history, "user", user_input);
        }

        // 创建 JSON 负载并发送请求
        char *json_payload = create_json_payload(history);
        debug("%s\n", json_payload);
        send_request(json_payload, &resp);
        debug("%s\n", resp.data);

        // 提取 AI 响应并打印
        char *ai_response = extract_ai_response(resp.data);
        if (ai_response != NULL)
        {
            if (strncmp(ai_response, "```json", 6) == 0 && strncmp(ai_response + strlen(ai_response) - 3, "```", 3) == 0)
            {
                // 计算并复制JSON字符串
                char *jsonStringStart = ai_response + 6 + 1;        // 跳过前面的```json和换行符
                size_t jsonStringLen = strlen(jsonStringStart) - 3; // 减去结尾的```
                char *jsonString = (char *)malloc(jsonStringLen + 1);
                if (jsonString == NULL)
                {
                    fprintf(stderr, "Memory allocation failed\n");
                    return 1;
                }
                strncpy(jsonString, jsonStringStart, jsonStringLen);
                jsonString[jsonStringLen] = '\0';
                debug("AI: %s\n", jsonString);
                add_message(&history, "assistant", jsonString);
                { // 解析json
                    cJSON *json = cJSON_Parse(jsonString);
                    if (json == NULL)
                    {
                        const char *error_ptr = cJSON_GetErrorPtr();
                        if (error_ptr != NULL)
                        {
                            fprintf(stderr, "解析错误之前: %s\n", error_ptr);
                        }
                        continue;
                    }

                    cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
                    if (cJSON_IsString(type) && (type->valuestring != NULL))
                    {
                        if (strcmp(type->valuestring, "控制指令") == 0)
                        {
                            process_control_command(json);
                        }
                        else if (strcmp(type->valuestring, "对话") == 0)
                        {
                            process_dialog(json);
                        }
                        else
                        {
                            printf("Unknown type: %s\n", type->valuestring);
                        }
                    }
                    cJSON_Delete(json);
                }
                free(ai_response);
                free(jsonString);
            }
            else
            {
                printf("AI: %s\n", ai_response);
                add_message(&history, "assistant", ai_response);
                free(ai_response);
            }
        }

        free(json_payload);
        free(resp.data);

        // 可以在这里添加退出条件
        if (strcmp(user_input, "exit") == 0)
            break;
    }

    free_messages(history);
    return 0;
}
