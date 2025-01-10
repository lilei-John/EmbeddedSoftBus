#ifndef SOFTBUS_MSG_H
#define SOFTBUS_MSG_H

#include <stdint.h>
#include <stddef.h>
#include "rbtree.h"
#include "device_ops.h"
#include "softbus_types.h"

// 消息结构体
typedef struct softbus_msg {
    uint32_t msg_id;
    char target[MAX_NAME_LENGTH];
    void* data;
    size_t data_len;
    softbus_cast_mode_t cast_mode;
    softbus_priority_t priority;
    struct rb_node node;
} softbus_msg_t;

#endif // SOFTBUS_MSG_H 