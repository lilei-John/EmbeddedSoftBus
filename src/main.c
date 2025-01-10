#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#endif
#include <signal.h>
#include "softbus.h"
#include "softbus_types.h"
#include "message_types.h"
#include "message_queue.h"

// 添加全局变量控制程序运行
static volatile int running = 1;

// 信号处理函数
static void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\nReceived Ctrl+C, shutting down...\n");
        running = 0;
    }
}

// 组消息响应回调函数
static void group_message_callback(const char* device_name, const char* response, int result, void* user_data) {
    printf("Group message response from %s: result=%d, response=%s\n",
           device_name, result, response ? response : "none");
}

// 温度传感器消息处理函数
static int temperature_sensor_handler(const char* msg, message_type_t type) {
    printf("Temperature sensor received message: %s (type: %d)\n", msg, type);
    
    // 只处理命令类型的消息，避免处理响应消息
    if (type != MESSAGE_TYPE_COMMAND) {
        return SOFTBUS_OK;
    }
    
    // 创建响应消息
    message_t response = {0};
    strncpy(response.target, "temperature_sensor", sizeof(response.target) - 1);
    response.type = MESSAGE_TYPE_RESPONSE;  // 将类型标记为响应
    response.priority = PRIORITY_HIGH;
    
    if (strcmp(msg, "get_temperature") == 0) {
        strcpy(response.content, "temperature:25.5C");
        printf("Temperature sensor response: %s\n", response.content);
    } else if (strcmp(msg, "status_check") == 0) {
        strcpy(response.content, "status:normal");
    } else if (strcmp(msg, "emergency_status") == 0) {
        strcpy(response.content, "emergency:none");
    } else {
        strcpy(response.content, "unknown_command");
    }
    
    // 分配消息数据内存
    response.data_len = strlen(response.content) + 1;
    response.data = malloc(response.data_len);
    if (!response.data) {
        printf("Failed to allocate memory for response data\n");
        return SOFTBUS_NO_MEM;
    }
    memcpy(response.data, response.content, response.data_len);
    
    // 发送响应消息
    int ret = message_queue_send(&response);
    if (ret != SOFTBUS_OK) {
        free(response.data);
        printf("Failed to send response message\n");
        return ret;
    }
    
    return SOFTBUS_OK;
}

// LED控制器消息处理函数
static int led_controller_handler(const char* msg, message_type_t type) {
    printf("LED controller received message: %s (type: %d)\n", msg, type);
    
    // 只处理命令类型的消息，避免处理响应消息
    if (type != MESSAGE_TYPE_COMMAND) {
        return SOFTBUS_OK;
    }
    
    // 创建响应消息
    message_t response = {0};
    strncpy(response.target, "led_controller", sizeof(response.target) - 1);
    response.type = MESSAGE_TYPE_RESPONSE;  // 将类型标记为响应
    response.priority = PRIORITY_HIGH;
    
    if (strncmp(msg, "set_brightness:", 14) == 0) {
        const char* brightness_str = msg + 14;
        printf("Debug: Raw brightness string: '%s'\n", brightness_str);
        
        // 跳过空格和冒号
        while (*brightness_str == ' ' || *brightness_str == ':') brightness_str++;
        printf("Debug: After skipping spaces and colon: '%s'\n", brightness_str);
        
        int brightness = atoi(brightness_str);
        printf("Debug: Parsed brightness value: %d\n", brightness);
        
        if (brightness < 0) {
            printf("Debug: Brightness was negative, setting to 0\n");
            brightness = 0;
        }
        if (brightness > 100) {
            printf("Debug: Brightness was over 100, setting to 100\n");
            brightness = 100;
        }
        
        // 使用实际解析的亮度值
        sprintf(response.content, "brightness_set:%d", brightness);
        printf("LED controller: Setting brightness to %d%%\n", brightness);
        printf("LED controller response: %s\n", response.content);
    } else if (strcmp(msg, "status_check") == 0) {
        strcpy(response.content, "status:on");
    } else if (strcmp(msg, "emergency_status") == 0) {
        strcpy(response.content, "emergency:none");
    } else {
        strcpy(response.content, "unknown_command");
    }
    
    // 分配消息数据内存
    response.data_len = strlen(response.content) + 1;
    response.data = malloc(response.data_len);
    if (!response.data) {
        printf("Failed to allocate memory for response data\n");
        return SOFTBUS_NO_MEM;
    }
    memcpy(response.data, response.content, response.data_len);
    
    // 发送响应消息
    int ret = message_queue_send(&response);
    if (ret != SOFTBUS_OK) {
        free(response.data);
        printf("Failed to send response message\n");
        return ret;
    }
    
    return SOFTBUS_OK;
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock\n");
        return 1;
    }
#endif

    int ret;

    // 设置信号处理
    signal(SIGINT, signal_handler);

    // 初始化软总线
    printf("Initializing softbus...\n");
    ret = softbus_api_init();
    if (ret != SOFTBUS_OK) {
        printf("Failed to initialize softbus\n");
        return 1;
    }

    // 注册设备
    printf("\nRegistering devices...\n");
    ret = softbus_api_register_device(DEVICE_TYPE_SENSOR, "temperature_sensor", temperature_sensor_handler);
    if (ret != SOFTBUS_OK) {
        printf("Failed to register temperature sensor\n");
        goto cleanup;
    }

    ret = softbus_api_register_device(DEVICE_TYPE_ACTUATOR, "led_controller", led_controller_handler);
    if (ret != SOFTBUS_OK) {
        printf("Failed to register LED controller\n");
        goto cleanup;
    }

    // 创建设备组
    printf("\nCreating device group...\n");
    ret = softbus_api_create_group("room1_devices");
    if (ret != SOFTBUS_OK) {
        printf("Failed to create device group\n");
        goto cleanup;
    }

    // 添加设备到组
    printf("\nAdding devices to group...\n");
    ret = softbus_api_add_to_group("room1_devices", "temperature_sensor");
    if (ret != SOFTBUS_OK) {
        printf("Failed to add temperature sensor to group\n");
        goto cleanup;
    }

    ret = softbus_api_add_to_group("room1_devices", "led_controller");
    if (ret != SOFTBUS_OK) {
        printf("Failed to add LED controller to group\n");
        goto cleanup;
    }

    // 测试各种消息发送
    printf("\nTesting message sending...\n");

    // 1. 发送温度查询消息
    printf("\n1. Querying temperature sensor:\n");
    ret = softbus_api_send_message_ex(
        "temperature_sensor",
        MESSAGE_TYPE_COMMAND,
        "get_temperature",
        PRIORITY_HIGH,
        SOFTBUS_MODE_SYNC,
        5000  // 增加超时时间到5秒
    );
    if (ret != SOFTBUS_OK) {
        printf("Failed to send temperature query, error: %d\n", ret);
        goto cleanup;
    }
    printf("Temperature query sent successfully\n");
    
#ifdef _WIN32
    Sleep(2000); // 增加等待时间到2秒
#else
    sleep(2);
#endif

    // 处理温度传感器的消息
    ret = softbus_api_process_messages("temperature_sensor");
    printf("Processed %d messages for temperature sensor\n", ret);

    // 2. 发送LED控制消息
    printf("\n2. Setting LED brightness:\n");
    ret = softbus_api_send_message_ex(
        "led_controller",
        MESSAGE_TYPE_COMMAND,
        "set_brightness:75",
        PRIORITY_NORMAL,
        SOFTBUS_MODE_SYNC,
        5000  // 增加超时时间到5秒
    );
    if (ret != SOFTBUS_OK) {
        printf("Failed to send LED control message, error: %d\n", ret);
        goto cleanup;
    }
    printf("LED control message sent successfully\n");

#ifdef _WIN32
    Sleep(2000); // 增加等待时间到2秒
#else
    sleep(2);
#endif

    // 处理LED控制器的消息
    ret = softbus_api_process_messages("led_controller");
    printf("Processed %d messages for LED controller\n", ret);

    // 3. 发送组消息
    printf("\n3. Sending group message:\n");
    ret = softbus_api_send_group_message_ex(
        "room1_devices",
        MESSAGE_TYPE_STATUS,
        "status_check",
        PRIORITY_HIGH,
        SOFTBUS_MODE_SYNC,
        5000,  // 增加超时时间到5秒
        group_message_callback,
        NULL
    );
    if (ret != SOFTBUS_OK) {
        printf("Failed to send group message, error: %d\n", ret);
        goto cleanup;
    }
    printf("Group message sent successfully\n");

#ifdef _WIN32
    Sleep(3000); // 增加等待时间到3秒
#else
    sleep(3);
#endif

    // 处理所有设备的消息
    printf("\nProcessing final messages:\n");
    ret = softbus_api_process_messages("temperature_sensor");
    printf("Processed %d messages for temperature sensor\n", ret);
    ret = softbus_api_process_messages("led_controller");
    printf("Processed %d messages for LED controller\n", ret);

cleanup:
    printf("\nCleaning up...\n");
    softbus_api_deinit();

#ifdef _WIN32
    WSACleanup();
#endif
    return (ret == SOFTBUS_OK) ? 0 : 1;
} 