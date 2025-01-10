#ifndef SOFTBUS_INTERNAL_H
#define SOFTBUS_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "rbtree.h"
#include "device_ops.h"
#include "softbus_types.h"
#include "device_manager.h"
#include "softbus_msg.h"

#define MAX_DEVICES 32
#define MAX_GROUPS 16
#define MAX_GROUP_MEMBERS 16

// 组管理结构体
typedef struct {
    char name[MAX_NAME_LENGTH];
    char members[MAX_GROUP_MEMBERS][MAX_NAME_LENGTH];
    int member_count;
} group_manager_t;

#endif // SOFTBUS_INTERNAL_H 