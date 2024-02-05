#include <stdio.h>
#include <stdlib.h>
#include <cJSON.h>

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

int main() {
    // 新的 JSON 字符串示例，可以是对话也可以是控制指令
    char *jsonStrings[] = {
        "{\"type\": \"控制指令\", \"operation\": \"switchLight\", \"parameters\": {\"status\": \"on\"}}",
        "{\"type\": \"控制指令\", \"operation\": \"activateFusion\", \"parameters\": {\"status\": \"start\"}}",
        "{\"type\": \"对话\", \"message\": \"您好，我是贾维斯，钢铁侠控制中心。我在这里帮助您控制设备和进行日常交流。\"}"
    };

    for (int i = 0; i < sizeof(jsonStrings) / sizeof(jsonStrings[0]); i++) {
        cJSON *json = cJSON_Parse(jsonStrings[i]);
        if (json == NULL) {
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL) {
                fprintf(stderr, "解析错误之前: %s\n", error_ptr);
            }
            continue;
        }

        cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
        if (cJSON_IsString(type) && (type->valuestring != NULL)) {
            if (strcmp(type->valuestring, "控制指令") == 0) {
                process_control_command(json);
            } else if (strcmp(type->valuestring, "对话") == 0) {
                process_dialog(json);
            } else {
                printf("Unknown type: %s\n", type->valuestring);
            }
        }

        cJSON_Delete(json);
    }

    return 0;
}
