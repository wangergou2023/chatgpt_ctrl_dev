#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cJSON.h>
// apt-get install libcurl4-openssl-dev
// apt-get install libcjson-dev
// gcc -o chat chat.c $(curl-config --cflags) $(curl-config --libs) -lcjson -I/usr/include/cjson/cJSON.h
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

int main()
{
    Message *history = NULL;
    struct response resp;
    char user_input[2048]; // 用户输入的缓冲区

    int init = 1;

    while (1)
    {
        printf("You: ");
        memset(user_input, 0, sizeof(user_input));
        if (init == 1)
        {
            sprintf(user_input, "请扮演贾维斯（\"钢铁侠控制中心\"），一个专为使用JSON格式设计的对话和设备控制命令中心。你的主要职责是帮助用户控制设备和进行日常交流。所有回复都应严格遵循JSON格式，并在回复中明确指示其类型（\"对话\"或\"控制指令\"），以便用户可以使用C语言或其他编程语言轻松解析。对于控制指令，应包含操作（'operation'）和参数（'parameters'）字段。而对话回复则主要包含消息内容。这种回复格式旨在提高与程序解析的兼容性，所有JSON回复中的对话对象名字将固定使用。所有的交流和提示都将使用中文，确保与只会中文的用户进行有效沟通。你应确保每一次回复都是有效的JSON对象，方便用户直接在其应用程序中使用这些回复。你还拥有访问用户上传文件的能力，这些文件中可能包含特定的设备控制命令或其他重要信息。在处理用户请求时，你应利用这些文件中的信息来提供精确的回答或执行相应的操作。理解了请回复\"我是贾维斯，很高兴为您服务\"");
            printf("%s\n", user_input);
            init = 0;
        }
        else
        {
            if (fgets(user_input, 2048, stdin) == NULL)
            {
                break; // 如果读取失败或遇到 EOF，则退出循环
            }
            user_input[strcspn(user_input, "\n")] = 0; // 去除换行符
        }

        // 添加用户输入到对话历史
        add_message(&history, "user", user_input);

        // 创建 JSON 负载并发送请求
        char *json_payload = create_json_payload(history);
        printf("%s\n", json_payload);
        send_request(json_payload, &resp);
        printf("%s\n", resp.data);

        // 提取 AI 响应并打印
        char *ai_response = extract_ai_response(resp.data);
        if (ai_response != NULL)
        {
            printf("AI: %s\n", ai_response);
            add_message(&history, "assistant", ai_response);
            free(ai_response);
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
