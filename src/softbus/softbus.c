#include <string.h>
#include <stdlib.h>
#include "softbus.h"
#include "rbtree.h"
#include "softbus_internal.h"
#include "softbus_socket.h"

/* 全局变量 */
static device_manager_t g_devices[MAX_DEVICES];
static group_manager_t g_groups[MAX_GROUPS];
static int g_device_count = 0;
static int g_group_count = 0;
static uint32_t g_msg_id_counter = 0;
static int g_is_initialized = 0;
static softbus_cast_mode_t g_cast_mode = SOFTBUS_UNICAST;

/* 内部函数声明 */
static device_manager_t* find_device(const char* name);
static group_manager_t* find_group(const char* name);
static uint32_t generate_msg_id(void);
static int softbus_send_multicast_msg(const char* group_name, void* data, size_t len, softbus_priority_t prio);
static int softbus_send_msg_prio(const char* target, void* data, size_t len, softbus_priority_t prio);

int softbus_init(void) {
    if (g_is_initialized) {
        return SOFTBUS_OK;
    }
    
    memset(g_devices, 0, sizeof(g_devices));
    memset(g_groups, 0, sizeof(g_groups));
    g_device_count = 0;
    g_group_count = 0;
    g_msg_id_counter = 0;
    g_is_initialized = 1;
    
#if ENABLE_SOCKET_MULTICAST
    // 初始化socket组播
    int ret = socket_multicast_init();
    if (ret != SOFTBUS_OK) {
        printf("Failed to initialize socket multicast\n");
        return ret;
    }

    // 启动组播接收线程
    ret = socket_multicast_start_receiver();
    if (ret != SOFTBUS_OK) {
        printf("Failed to start multicast receiver\n");
        socket_multicast_deinit();
        return ret;
    }
#endif
    
    return SOFTBUS_OK;
}

int softbus_deinit(void) {
    if (!g_is_initialized) {
        return SOFTBUS_ERROR;
    }
    
#if ENABLE_SOCKET_MULTICAST
    // 停止组播接收线程
    socket_multicast_stop_receiver();
    // 清理socket组播
    socket_multicast_deinit();
#endif

    for (int i = 0; i < g_device_count; i++) {
        if (g_devices[i].ops.deinit) {
            g_devices[i].ops.deinit(g_devices[i].private_data);
        }
        
        // 清理消息树中的所有消息
        struct rb_node* node;
        while ((node = rb_first(&g_devices[i].msg_tree)) != NULL) {
            softbus_msg_t* msg = rb_entry(node, softbus_msg_t, node);
            rb_erase(node, &g_devices[i].msg_tree);
            free(msg->data);
            free(msg);
        }
    }
    
    g_is_initialized = 0;
    return SOFTBUS_OK;
}

int softbus_register_device(const char* name, device_ops_t* ops) {
    if (!g_is_initialized || !name || !ops || g_device_count >= MAX_DEVICES) {
        return SOFTBUS_ERROR;
    }
    
    device_manager_t* dev = find_device(name);
    if (dev) {
        // 如果设备已存在，更新其操作函数
        memcpy(&dev->ops, ops, sizeof(device_ops_t));
        return SOFTBUS_OK;
    }
    
    // 添加新设备
    dev = &g_devices[g_device_count];
    strncpy(dev->name, name, MAX_NAME_LENGTH - 1);
    dev->name[MAX_NAME_LENGTH - 1] = '\0';
    memcpy(&dev->ops, ops, sizeof(device_ops_t));
    dev->private_data = NULL;
    dev->msg_callback = NULL;
    dev->msg_tree = RB_ROOT;  // 初始化红黑树
    
    if (dev->ops.init && dev->ops.init(dev->private_data) != SOFTBUS_OK) {
        return SOFTBUS_ERROR;
    }
    
    g_device_count++;
    return SOFTBUS_OK;
}

int softbus_unregister_device(const char* name) {
    if (!g_is_initialized || !name) {
        return SOFTBUS_ERROR;
    }
    
    device_manager_t* dev = find_device(name);
    if (!dev) {
        return SOFTBUS_NOT_FOUND;
    }
    
    if (dev->ops.deinit) {
        dev->ops.deinit(dev->private_data);
    }
    
    // 清理消息树中的所有消息
    struct rb_node* node;
    while ((node = rb_first(&dev->msg_tree)) != NULL) {
        softbus_msg_t* msg = rb_entry(node, softbus_msg_t, node);
        rb_erase(node, &dev->msg_tree);
        free(msg->data);
        free(msg);
    }
    
    // 移动设备列表以填补空缺
    int idx = dev - g_devices;
    for (int i = idx; i < g_device_count - 1; i++) {
        memcpy(&g_devices[i], &g_devices[i + 1], sizeof(device_manager_t));
    }
    
    g_device_count--;
    return SOFTBUS_OK;
}

int softbus_send_msg(const char* target, void* data, size_t len) {
    return softbus_send_msg_prio(target, data, len, PRIORITY_NORMAL);
}

static int softbus_send_msg_prio(const char* target, void* data, size_t len, softbus_priority_t prio) {
    if (!g_is_initialized || !target || !data) {
        return SOFTBUS_ERROR;
    }
    
    // 如果当前是组播模式且目标是组名，则使用组播发送
    if (g_cast_mode == SOFTBUS_MULTICAST && find_group(target)) {
        return softbus_send_multicast_msg(target, data, len, prio);
    }
    
    // 否则使用单播发送
    device_manager_t* dev = device_manager_find(target);  // 使用device_manager中的查找函数
    if (!dev) {
        return SOFTBUS_NOT_FOUND;
    }
    
    // 处理消息
    if (dev->ops.process_msg) {
        softbus_msg_t* msg = (softbus_msg_t*)data;
        return dev->ops.process_msg(dev->private_data, msg->data, msg->data_len, 
                                  prio == PRIORITY_HIGH ? MESSAGE_TYPE_COMMAND : MESSAGE_TYPE_DATA);
    }
    
    return SOFTBUS_ERROR;  // 没有消息处理函数
}

static int softbus_send_multicast_msg(const char* group_name, void* data, size_t len, 
                              softbus_priority_t prio) {
    if (!g_is_initialized || !group_name || !data) {
        return SOFTBUS_ERROR;
    }
    
    group_manager_t* group = find_group(group_name);
    if (!group) {
        return SOFTBUS_NOT_FOUND;
    }
    
    int success_count = 0;
    
    // 向组内每个设备发送消息
    for (int i = 0; i < group->member_count; i++) {
        device_manager_t* dev = device_manager_find(group->members[i]);
        if (!dev) {
            continue;
        }
        
        // 处理消息
        if (dev->ops.process_msg) {
            softbus_msg_t* msg = (softbus_msg_t*)data;
            int ret = dev->ops.process_msg(dev->private_data, msg->data, msg->data_len,
                                         prio == PRIORITY_HIGH ? MESSAGE_TYPE_COMMAND : MESSAGE_TYPE_DATA);
            if (ret == SOFTBUS_OK) {
                success_count++;
            }
        }
    }
    
    return (success_count > 0) ? SOFTBUS_OK : SOFTBUS_ERROR;
}

/* 内部辅助函数实现 */
static device_manager_t* find_device(const char* name) {
    for (int i = 0; i < g_device_count; i++) {
        if (strcmp(g_devices[i].name, name) == 0) {
            return &g_devices[i];
        }
    }
    return NULL;
}

static group_manager_t* find_group(const char* name) {
    for (int i = 0; i < g_group_count; i++) {
        if (strcmp(g_groups[i].name, name) == 0) {
            return &g_groups[i];
        }
    }
    return NULL;
}

static uint32_t generate_msg_id(void) {
    return ++g_msg_id_counter;
} 