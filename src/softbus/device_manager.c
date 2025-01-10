#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "device_manager.h"
#include "softbus_types.h"
#include "message_types.h"
#include "message_queue.h"
#include "rbtree.h"

#define MAX_DEVICES 32

// 设备列表
static struct {
    device_manager_t devices[MAX_DEVICES];
    int count;
    pthread_mutex_t mutex;
} g_device_manager;

// 初始化设备管理器
int device_manager_init(void) {
    printf("Initializing device manager...\n");
    memset(&g_device_manager, 0, sizeof(g_device_manager));
    pthread_mutex_init(&g_device_manager.mutex, NULL);
    return SOFTBUS_OK;
}

// 清理设备管理器
void device_manager_deinit(void) {
    printf("Cleaning up device manager...\n");
    pthread_mutex_lock(&g_device_manager.mutex);
    for (int i = 0; i < g_device_manager.count; i++) {
        device_manager_t* device = &g_device_manager.devices[i];
        printf("Cleaning up device: %s\n", device->name);
        if (device->ops.deinit) {
            device->ops.deinit(device->private_data);
        }
        // 清理消息树
        struct rb_node* node;
        while ((node = rb_first(&device->msg_tree)) != NULL) {
            message_t* msg = rb_entry(node, message_t, node);
            rb_erase(node, &device->msg_tree);
            if (msg->data) {
                free(msg->data);
            }
            free(msg);
        }
    }
    g_device_manager.count = 0;
    pthread_mutex_unlock(&g_device_manager.mutex);
    pthread_mutex_destroy(&g_device_manager.mutex);
}

// 注册设备
int device_manager_register(device_manager_t* device) {
    if (!device) {
        return SOFTBUS_INVALID_ARG;
    }

    printf("Registering device: %s (type: %d)\n", device->name, device->type);
    pthread_mutex_lock(&g_device_manager.mutex);

    // 检查设备是否已存在
    for (int i = 0; i < g_device_manager.count; i++) {
        if (strcmp(g_device_manager.devices[i].name, device->name) == 0) {
            printf("Device already exists: %s\n", device->name);
            pthread_mutex_unlock(&g_device_manager.mutex);
            return SOFTBUS_ERROR;
        }
    }

    // 检查是否达到最大设备数
    if (g_device_manager.count >= MAX_DEVICES) {
        printf("Maximum device limit reached\n");
        pthread_mutex_unlock(&g_device_manager.mutex);
        return SOFTBUS_ERROR;
    }

    // 添加设备
    device_manager_t* new_device = &g_device_manager.devices[g_device_manager.count];
    memcpy(new_device, device, sizeof(device_manager_t));
    
    // 初始化消息树
    new_device->msg_tree.rb_node = NULL;
    
    // 如果有初始化函数，调用它
    if (new_device->ops.init) {
        int ret = new_device->ops.init(new_device->private_data);
        if (ret != SOFTBUS_OK) {
            printf("Failed to initialize device: %s\n", device->name);
            pthread_mutex_unlock(&g_device_manager.mutex);
            return ret;
        }
    }

    g_device_manager.count++;
    printf("Device registered successfully: %s\n", device->name);

    pthread_mutex_unlock(&g_device_manager.mutex);
    return SOFTBUS_OK;
}

// 注销设备
int device_manager_unregister(const char* device_name) {
    if (!device_name) {
        return SOFTBUS_INVALID_ARG;
    }

    printf("Unregistering device: %s\n", device_name);
    pthread_mutex_lock(&g_device_manager.mutex);

    // 查找设备
    int idx = -1;
    for (int i = 0; i < g_device_manager.count; i++) {
        if (strcmp(g_device_manager.devices[i].name, device_name) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        printf("Device not found: %s\n", device_name);
        pthread_mutex_unlock(&g_device_manager.mutex);
        return SOFTBUS_NOT_FOUND;
    }

    // 调用设备清理函数
    device_manager_t* device = &g_device_manager.devices[idx];
    if (device->ops.deinit) {
        device->ops.deinit(device->private_data);
    }

    // 清理消息树
    struct rb_node* node;
    while ((node = rb_first(&device->msg_tree)) != NULL) {
        message_t* msg = rb_entry(node, message_t, node);
        rb_erase(node, &device->msg_tree);
        if (msg->data) {
            free(msg->data);
        }
        free(msg);
    }

    // 移动设备列表以填补空缺
    for (int i = idx; i < g_device_manager.count - 1; i++) {
        memcpy(&g_device_manager.devices[i], &g_device_manager.devices[i + 1], sizeof(device_manager_t));
    }

    g_device_manager.count--;
    printf("Device unregistered successfully: %s\n", device_name);
    pthread_mutex_unlock(&g_device_manager.mutex);
    return SOFTBUS_OK;
}

// 查找设备
device_manager_t* device_manager_find(const char* device_name) {
    if (!device_name) {
        return NULL;
    }

    pthread_mutex_lock(&g_device_manager.mutex);
    for (int i = 0; i < g_device_manager.count; i++) {
        if (strcmp(g_device_manager.devices[i].name, device_name) == 0) {
            pthread_mutex_unlock(&g_device_manager.mutex);
            return &g_device_manager.devices[i];
        }
    }
    pthread_mutex_unlock(&g_device_manager.mutex);
    printf("Device not found: %s\n", device_name);
    return NULL;
}

// 检查设备是否已注册
bool device_manager_is_device_registered(const char* device_name) {
    return device_manager_find(device_name) != NULL;
} 