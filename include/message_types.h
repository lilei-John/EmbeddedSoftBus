#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <time.h>
#include "softbus_types.h"
#include "rbtree.h"

// 消息结构体定义
typedef struct {
    struct rb_node node;        // 红黑树节点
    char target[32];           // 目标设备
    message_type_t type;       // 消息类型
    softbus_priority_t priority; // 优先级
    char content[1024];        // 消息内容
    void* data;                // 消息数据
    size_t data_len;           // 数据长度
    struct timespec timestamp; // 时间戳
} message_t;

// 消息回调函数类型
typedef void (*message_callback_t)(const char* target, int result, void* user_data);

#endif // MESSAGE_TYPES_H 