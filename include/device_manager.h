#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "softbus_types.h"
#include "device_ops.h"
#include "rbtree.h"

// 设备管理器结构体
typedef struct {
    char name[MAX_NAME_LENGTH];
    device_type_t type;
    device_ops_t ops;
    void* private_data;
    struct rb_root msg_tree;
    void (*msg_callback)(void* msg);
} device_manager_t;

// 设备管理器API
int device_manager_init(void);
void device_manager_deinit(void);
int device_manager_register(device_manager_t* device);
int device_manager_unregister(const char* device_name);
device_manager_t* device_manager_find(const char* device_name);
bool device_manager_is_device_registered(const char* device_name);

#endif // DEVICE_MANAGER_H 