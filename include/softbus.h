#ifndef SOFTBUS_H
#define SOFTBUS_H

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include "softbus_types.h"
#include "message_types.h"
#include "device_manager.h"

// 基础API函数
int softbus_init(void);
int softbus_deinit(void);
int softbus_register_device(const char* name, device_ops_t* ops);
int softbus_unregister_device(const char* name);
int softbus_send_msg(const char* target, void* data, size_t len);

// 组消息响应回调函数类型
typedef void (*group_message_callback_t)(const char* device_name, const char* response, int result, void* user_data);

// API functions
int softbus_api_init(void);
void softbus_api_deinit(void);
int softbus_api_register_device(device_type_t type, const char* device_name,
                              int (*handler)(const char* msg, message_type_t type));

// 设备管理
int softbus_api_unregister_device(const char* device_name);

// 组管理
int softbus_api_create_group(const char* group_name);
int softbus_api_delete_group(const char* group_name);
int softbus_api_add_to_group(const char* group_name, const char* device_name);
int softbus_api_remove_from_group(const char* group_name, const char* device_name);

// 消息发送API
int softbus_api_send_message_ex(const char* target, message_type_t type,
                              const char* message, softbus_priority_t priority,
                              softbus_mode_t mode, int timeout_ms);

// 组消息发送API
int softbus_api_send_group_message_ex(const char* group_name, message_type_t type,
                                    const char* message, softbus_priority_t priority,
                                    softbus_mode_t mode, int timeout_ms,
                                    group_message_callback_t callback, void* user_data);

// 消息查询
int softbus_api_get_pending_messages(const char* device_name, message_t* msgs, int* count);
int softbus_api_process_messages(const char* device_name);

// 状态查询
bool softbus_api_is_device_registered(const char* device_name);
bool softbus_api_is_group_exists(const char* group_name);
int softbus_api_get_group_devices(const char* group_name, char** device_names, int* count);

// 向后兼容的函数声明
static inline int softbus_api_send_message(const char* target, message_type_t type,
                                         const char* message, softbus_priority_t priority) {
    return softbus_api_send_message_ex(target, type, message, priority,
                                     SOFTBUS_MODE_ASYNC, 0);
}

static inline int softbus_api_send_group_message(const char* group_name, message_type_t type,
                                               const char* message, softbus_priority_t priority) {
    return softbus_api_send_group_message_ex(group_name, type, message, priority,
                                           SOFTBUS_MODE_ASYNC, 0, NULL, NULL);
}

#endif // SOFTBUS_H 