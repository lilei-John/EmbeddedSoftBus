#ifndef DEVICE_OPS_H
#define DEVICE_OPS_H

#include <stddef.h>
#include <stdbool.h>
#include "softbus_types.h"

#define MAX_NAME_LENGTH 64
#define MAX_MSG_LENGTH 1024

// 设备操作函数类型定义
typedef struct {
    int (*init)(void* private_data);
    void (*deinit)(void* private_data);
    int (*start)(void* private_data);
    int (*stop)(void* private_data);
    int (*process_msg)(void* private_data, const void* msg, size_t len, message_type_t type);
} device_ops_t;

#endif // DEVICE_OPS_H 