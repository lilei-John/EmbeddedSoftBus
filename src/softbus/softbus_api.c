#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "softbus.h"
#include "softbus_internal.h"
#include "device_manager.h"
#include "message_queue.h"
#include "softbus_socket.h"

// 内部函数声明
static device_manager_t* find_device(const char* device_name);
static group_manager_t* find_group(const char* group_name);
static int msg_handler_wrapper(void* private_data, const void* data, size_t len, message_type_t type);
static void message_complete_callback(const char* target, int result, void* user_data);

// 全局变量
static group_manager_t g_groups[MAX_GROUPS];
static int g_group_count = 0;
static pthread_mutex_t g_groups_mutex = PTHREAD_MUTEX_INITIALIZER;

// 同步等待结构
typedef struct {
    sem_t sem;
    int result;
    bool completed;
    char response[1024];  // 添加响应消息内容
} sync_wait_t;

// 消息处理包装函数实现
static int msg_handler_wrapper(void* private_data, const void* data, size_t len, message_type_t type) {
    const char* msg = (const char*)data;
    int (*handler)(const char*, message_type_t) = (int (*)(const char*, message_type_t))private_data;
    
    printf("Message handler wrapper called with message: %s\n", msg);
    if (!handler) {
        printf("Error: No message handler registered\n");
        return SOFTBUS_ERROR;
    }
    
    int result = handler(msg, type);
    printf("Message handler result: %d\n", result);
    
    return result;
}

int softbus_api_init(void) {
    // 初始化互斥锁
    if (pthread_mutex_init(&g_groups_mutex, NULL) != 0) {
        return SOFTBUS_ERROR;
    }

    // 初始化组管理器
    g_group_count = 0;
    memset(g_groups, 0, sizeof(g_groups));

    // 初始化消息队列
    int ret = message_queue_init();
    if (ret != SOFTBUS_OK) {
        pthread_mutex_destroy(&g_groups_mutex);
        return ret;
    }

    // 初始化设备管理器
    ret = device_manager_init();
    if (ret != SOFTBUS_OK) {
        message_queue_deinit();
        pthread_mutex_destroy(&g_groups_mutex);
        return ret;
    }

    // 初始化软总线系统
    ret = softbus_init();
    if (ret != SOFTBUS_OK) {
        device_manager_deinit();
        message_queue_deinit();
        pthread_mutex_destroy(&g_groups_mutex);
        return ret;
    }

    return SOFTBUS_OK;
}

void softbus_api_deinit(void) {
    // 注销所有设备
    for (int i = 0; i < g_group_count; i++) {
        for (int j = 0; j < g_groups[i].member_count; j++) {
            softbus_api_unregister_device(g_groups[i].members[j]);
        }
    }

    // 清理所有组
    pthread_mutex_lock(&g_groups_mutex);
    g_group_count = 0;
    memset(g_groups, 0, sizeof(g_groups));
    pthread_mutex_unlock(&g_groups_mutex);

    // 清理软总线系统
    softbus_deinit();

    // 清理设备管理器
    device_manager_deinit();

    // 清理消息队列
    message_queue_deinit();

    // 销毁互斥锁
    pthread_mutex_destroy(&g_groups_mutex);
}

int softbus_api_register_device(device_type_t type, const char* device_name,
                                int (*handler)(const char* msg, message_type_t type)) {
    if (!device_name || !handler) {
        return SOFTBUS_INVALID_ARG;
    }

    device_manager_t device = {0};
    strncpy(device.name, device_name, MAX_NAME_LENGTH - 1);
    device.name[MAX_NAME_LENGTH - 1] = '\0';
    device.type = type;
    device.msg_tree.rb_node = NULL;  // 初始化红黑树根节点
    device.msg_callback = NULL;
    
    // 初始化设备操作函数
    device.ops.init = NULL;
    device.ops.deinit = NULL;
    device.ops.process_msg = msg_handler_wrapper;  // 设置消息处理函数
    device.private_data = handler;  // 存储消息处理回调

    int ret = device_manager_register(&device);
    if (ret == SOFTBUS_OK) {
        // 注册到softbus系统
        device_ops_t ops = {
            .init = NULL,
            .deinit = NULL,
            .process_msg = msg_handler_wrapper
        };
        ret = softbus_register_device(device_name, &ops);
        
        if (ret == SOFTBUS_OK) {
            // 设置消息队列回调
            message_queue_set_callback(device_name, message_complete_callback, NULL);
            
            // 启动消息处理
            device_manager_t* dev = device_manager_find(device_name);
            if (dev) {
                // 处理任何待处理的消息
                softbus_api_process_messages(device_name);
            }
        }
    }
    return ret;
}

int softbus_api_unregister_device(const char* device_name) {
    if (!device_name) {
        return SOFTBUS_INVALID_ARG;
    }

    // 首先处理所有待处理的消息
    softbus_api_process_messages(device_name);

    // 移除消息队列回调
    message_queue_remove_callback(device_name);

    // 从所有组中移除设备
    pthread_mutex_lock(&g_groups_mutex);
    for (int i = 0; i < g_group_count; i++) {
        for (int j = 0; j < g_groups[i].member_count; j++) {
            if (strcmp(g_groups[i].members[j], device_name) == 0) {
                // 移动成员列表以填补空缺
                for (int k = j; k < g_groups[i].member_count - 1; k++) {
                    strcpy(g_groups[i].members[k], g_groups[i].members[k + 1]);
                }
                g_groups[i].member_count--;
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_groups_mutex);

    // 获取设备管理器
    device_manager_t* device = device_manager_find(device_name);
    if (device) {
        // 调用设备的清理函数
        if (device->ops.deinit) {
            device->ops.deinit(device->private_data);
        }
        
        // 清理设备的私有数据
        if (device->private_data) {
            free(device->private_data);
            device->private_data = NULL;
        }
    }

    // 从设备管理器中注销设备
    int ret = device_manager_unregister(device_name);
    if (ret != SOFTBUS_OK) {
        return ret;
    }

    // 从softbus系统中注销设备
    return softbus_unregister_device(device_name);
}

// 组管理函数实现
int softbus_api_create_group(const char* group_name) {
    if (!group_name) {
        return SOFTBUS_INVALID_ARG;
    }

    pthread_mutex_lock(&g_groups_mutex);

    if (g_group_count >= MAX_GROUPS) {
        pthread_mutex_unlock(&g_groups_mutex);
        return SOFTBUS_ERROR;
    }

    // 检查组是否已存在
    for (int i = 0; i < g_group_count; i++) {
        if (strcmp(g_groups[i].name, group_name) == 0) {
            pthread_mutex_unlock(&g_groups_mutex);
            return SOFTBUS_ERROR;
        }
    }

    // 创建新组
    strncpy(g_groups[g_group_count].name, group_name, MAX_NAME_LENGTH - 1);
    g_groups[g_group_count].name[MAX_NAME_LENGTH - 1] = '\0';
    g_groups[g_group_count].member_count = 0;
    g_group_count++;

    pthread_mutex_unlock(&g_groups_mutex);
    return SOFTBUS_OK;
}

int softbus_api_delete_group(const char* group_name) {
    if (!group_name) {
        return SOFTBUS_INVALID_ARG;
    }

    pthread_mutex_lock(&g_groups_mutex);

    // 查找组
    int idx = -1;
    for (int i = 0; i < g_group_count; i++) {
        if (strcmp(g_groups[i].name, group_name) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        pthread_mutex_unlock(&g_groups_mutex);
        return SOFTBUS_NOT_FOUND;
    }

    // 移动组列表以填补空缺
    for (int i = idx; i < g_group_count - 1; i++) {
        memcpy(&g_groups[i], &g_groups[i + 1], sizeof(group_manager_t));
    }

    g_group_count--;
    pthread_mutex_unlock(&g_groups_mutex);
    return SOFTBUS_OK;
}

int softbus_api_add_to_group(const char* group_name, const char* device_name) {
    if (!group_name || !device_name) {
        return SOFTBUS_INVALID_ARG;
    }

    pthread_mutex_lock(&g_groups_mutex);

    // 查找组
    int group_idx = -1;
    for (int i = 0; i < g_group_count; i++) {
        if (strcmp(g_groups[i].name, group_name) == 0) {
            group_idx = i;
            break;
        }
    }

    if (group_idx == -1) {
        pthread_mutex_unlock(&g_groups_mutex);
        return SOFTBUS_NOT_FOUND;
    }

    group_manager_t* group = &g_groups[group_idx];

    // 检查设备是否已在组中
    for (int i = 0; i < group->member_count; i++) {
        if (strcmp(group->members[i], device_name) == 0) {
            pthread_mutex_unlock(&g_groups_mutex);
            return SOFTBUS_OK;
        }
    }

    // 检查组是否已满
    if (group->member_count >= MAX_GROUP_MEMBERS) {
        pthread_mutex_unlock(&g_groups_mutex);
        return SOFTBUS_ERROR;
    }

    // 检查设备是否存在
    if (!device_manager_is_device_registered(device_name)) {
        pthread_mutex_unlock(&g_groups_mutex);
        return SOFTBUS_NOT_FOUND;
    }

    // 添加设备到组
    strncpy(group->members[group->member_count], device_name, MAX_NAME_LENGTH - 1);
    group->members[group->member_count][MAX_NAME_LENGTH - 1] = '\0';
    group->member_count++;

    pthread_mutex_unlock(&g_groups_mutex);
    return SOFTBUS_OK;
}

int softbus_api_remove_from_group(const char* group_name, const char* device_name) {
    if (!group_name || !device_name) {
        return SOFTBUS_INVALID_ARG;
    }

    pthread_mutex_lock(&g_groups_mutex);

    // 查找组
    int group_idx = -1;
    for (int i = 0; i < g_group_count; i++) {
        if (strcmp(g_groups[i].name, group_name) == 0) {
            group_idx = i;
            break;
        }
    }

    if (group_idx == -1) {
        pthread_mutex_unlock(&g_groups_mutex);
        return SOFTBUS_NOT_FOUND;
    }

    group_manager_t* group = &g_groups[group_idx];

    // 查找设备
    int dev_idx = -1;
    for (int i = 0; i < group->member_count; i++) {
        if (strcmp(group->members[i], device_name) == 0) {
            dev_idx = i;
            break;
        }
    }

    if (dev_idx == -1) {
        pthread_mutex_unlock(&g_groups_mutex);
        return SOFTBUS_NOT_FOUND;
    }

    // 移动成员列表以填补空缺
    for (int i = dev_idx; i < group->member_count - 1; i++) {
        strcpy(group->members[i], group->members[i + 1]);
    }

    group->member_count--;
    pthread_mutex_unlock(&g_groups_mutex);
    return SOFTBUS_OK;
}

// 消息完成回调函数
static void message_complete_callback(const char* target, int result, void* user_data) {
    sync_wait_t* wait = (sync_wait_t*)user_data;
    if (wait) {
        wait->result = result;
        wait->completed = true;
        
        // 获取响应消息
        message_t response_msg;
        if (message_queue_peek(target, &response_msg) == SOFTBUS_OK) {
            strncpy(wait->response, response_msg.content, sizeof(wait->response) - 1);
            wait->response[sizeof(wait->response) - 1] = '\0';
            printf("Response received: %s\n", wait->response);
        }
        
        sem_post(&wait->sem);
    }
}

// 实现扩展的消息发送API
int softbus_api_send_message_ex(const char* target, message_type_t type,
                              const char* message, softbus_priority_t priority,
                              softbus_mode_t mode, int timeout_ms) {
    if (!target || !message) {
        return SOFTBUS_INVALID_ARG;
    }

    // 检查设备是否存在
    if (!device_manager_is_device_registered(target)) {
        return SOFTBUS_NOT_FOUND;
    }

    message_t msg = {0};
    strncpy(msg.target, target, sizeof(msg.target) - 1);
    msg.type = type;
    msg.priority = priority;
    strncpy(msg.content, message, sizeof(msg.content) - 1);
    msg.data_len = strlen(message) + 1;
    
    // 分配消息数据内存
    msg.data = malloc(msg.data_len);
    if (!msg.data) {
        return SOFTBUS_NO_MEM;
    }
    memcpy(msg.data, message, msg.data_len);
    
    clock_gettime(CLOCK_REALTIME, &msg.timestamp);

    int ret;
    if (mode == SOFTBUS_MODE_ASYNC) {
        // 异步模式：直接发送消息
        ret = message_queue_send(&msg);
        if (ret != SOFTBUS_OK) {
            free(msg.data);  // 发送失败时释放内存
            return ret;
        }
        // 立即处理消息
        softbus_api_process_messages(target);
        return ret;
    } else {
        // 同步模式：创建等待结构并等待完成
        sync_wait_t wait = {0};
        sem_init(&wait.sem, 0, 0);
        wait.completed = false;
        wait.result = SOFTBUS_ERROR;

        // 设置回调
        message_queue_set_callback(target, message_complete_callback, &wait);

        // 发送消息
        ret = message_queue_send(&msg);
        if (ret != SOFTBUS_OK) {
            free(msg.data);  // 发送失败时释放内存
            sem_destroy(&wait.sem);
            return ret;
        }

        // 立即处理消息
        softbus_api_process_messages(target);

        // 等待完成或超时
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        if (sem_timedwait(&wait.sem, &ts) == 0) {
            ret = wait.result;
            // 打印响应消息
            if (ret == SOFTBUS_OK && wait.response[0] != '\0') {
                printf("Received response from %s: %s\n", target, wait.response);
            }
        } else {
            ret = SOFTBUS_TIMEOUT;
        }

        // 清理
        message_queue_set_callback(target, NULL, NULL);
        sem_destroy(&wait.sem);
        return ret;
    }
}

// 实现扩展的组消息发送API
int softbus_api_send_group_message_ex(const char* group_name, message_type_t type,
                                    const char* message, softbus_priority_t priority,
                                    softbus_mode_t mode, int timeout_ms,
                                    group_message_callback_t callback, void* user_data) {
    if (!group_name || !message) {
        return SOFTBUS_INVALID_ARG;
    }

#if ENABLE_SOCKET_MULTICAST
    // 如果启用了socket组播，优先使用组播发送消息
    int ret = socket_multicast_send(message, strlen(message));
    if (ret == SOFTBUS_OK) {
        printf("Message sent via multicast: %s\n", message);
        if (callback) {
            callback("multicast", message, SOFTBUS_OK, user_data);
        }
        return SOFTBUS_OK;
    }
    // 如果组播发送失败，回退到普通的组消息发送
    printf("Multicast send failed, falling back to normal group message...\n");
#endif

    pthread_mutex_lock(&g_groups_mutex);

    // 查找组
    int group_idx = -1;
    for (int i = 0; i < g_group_count; i++) {
        if (strcmp(g_groups[i].name, group_name) == 0) {
            group_idx = i;
            break;
        }
    }

    if (group_idx == -1) {
        pthread_mutex_unlock(&g_groups_mutex);
        return SOFTBUS_NOT_FOUND;
    }

    group_manager_t* group = &g_groups[group_idx];
    int member_count = group->member_count;
    char device_names[MAX_GROUP_MEMBERS][MAX_NAME_LENGTH];
    memcpy(device_names, group->members, sizeof(device_names));

    pthread_mutex_unlock(&g_groups_mutex);

    // 同步模式下需要等待所有设备都处理完成并返回响应
    if (mode == SOFTBUS_MODE_SYNC) {
        sync_wait_t wait = {0};
        sem_init(&wait.sem, 0, 0);
        wait.completed = false;
        wait.result = SOFTBUS_OK;

        // 为每个设备发送消息并等待响应
        for (int i = 0; i < member_count; i++) {
            // 发送消息
            int ret = softbus_api_send_message_ex(device_names[i], type, message,
                                            priority, mode, timeout_ms);
            if (ret != SOFTBUS_OK) {
                wait.result = ret;
                if (callback) {
                    callback(device_names[i], NULL, ret, user_data);
                }
                continue;
            }

            // 等待响应消息
            device_manager_t* dev = device_manager_find(device_names[i]);
            if (dev && dev->ops.process_msg) {
                // 处理消息并获取响应
                ret = dev->ops.process_msg(dev->private_data, message, strlen(message) + 1, type);
                
                // 获取响应消息
                message_t response_msg;
                if (message_queue_receive(device_names[i], &response_msg) == SOFTBUS_OK) {
                    // 收到响应消息
                    if (callback) {
                        callback(device_names[i], response_msg.content, ret, user_data);
                    }
                    // 释放响应消息数据
                    if (response_msg.data) {
                        free(response_msg.data);
                        response_msg.data = NULL;
                    }
                } else {
                    // 没有收到响应消息
                    if (callback) {
                        callback(device_names[i], NULL, SOFTBUS_TIMEOUT, user_data);
                    }
                    wait.result = SOFTBUS_TIMEOUT;
                }
            }
        }

        sem_destroy(&wait.sem);
        return wait.result;
    } else {
        // 异步模式下直接发送给所有设备
        int final_ret = SOFTBUS_OK;
        for (int i = 0; i < member_count; i++) {
            int ret = softbus_api_send_message_ex(device_names[i], type, message,
                                            priority, mode, 0);
            if (ret != SOFTBUS_OK) {
                final_ret = ret;
                if (callback) {
                    callback(device_names[i], NULL, ret, user_data);
                }
            }
        }
        return final_ret;
    }
}

// 消息查询函数
int softbus_api_get_pending_messages(const char* device_name, message_t* msgs, int* count) {
    if (!device_name || !msgs || !count || *count <= 0) {
        return SOFTBUS_INVALID_ARG;
    }

    device_manager_t* dev = find_device(device_name);
    if (!dev) {
        return SOFTBUS_NOT_FOUND;
    }

    int msg_count = 0;
    struct rb_node* node = rb_first(&dev->msg_tree);
    
    while (node && msg_count < *count) {
        message_t* msg = rb_entry(node, message_t, node);
        memcpy(&msgs[msg_count], msg, sizeof(message_t));
        msg_count++;
        node = rb_next(node);
    }

    *count = msg_count;
    return SOFTBUS_OK;
}

// 消息处理函数
int softbus_api_process_messages(const char* device_name) {
    if (!device_name) {
        return SOFTBUS_INVALID_ARG;
    }

    printf("Processing messages for device: %s\n", device_name);
    
    message_t msg;
    int processed = 0;
    
    // 获取设备管理器
    device_manager_t* device = device_manager_find(device_name);
    if (!device) {
        printf("Device not found: %s\n", device_name);
        return SOFTBUS_NOT_FOUND;
    }
    
    // 处理所有待处理的消息
    while (message_queue_receive(device_name, &msg) == SOFTBUS_OK) {
        printf("Processing message for device %s: type=%d, content=%s\n",
               device_name, msg.type, msg.content);
        
        if (device->ops.process_msg) {
            int result = device->ops.process_msg(device->private_data, 
                                               msg.content, 
                                               strlen(msg.content) + 1, 
                                               msg.type);
            printf("Message handler returned: %d\n", result);
            
            // 如果是同步模式，等待响应
            if (msg.type == MESSAGE_TYPE_COMMAND) {
                message_t response;
                if (message_queue_receive(device_name, &response) == SOFTBUS_OK) {
                    printf("Response received from %s: %s\n", device_name, response.content);
                    if (response.data) {
                        free(response.data);
                    }
                }
            }
            
            processed++;
        } else {
            printf("No message handler found for device: %s\n", device_name);
        }
        
        // 释放消息数据
        if (msg.data) {
            free(msg.data);
            msg.data = NULL;
        }
    }
    
    printf("Processed %d messages for device %s\n", processed, device_name);
    return processed;
}

bool softbus_api_is_device_registered(const char* device_name) {
    if (!device_name) {
        return false;
    }
    return find_device(device_name) != NULL;
}

bool softbus_api_is_group_exists(const char* group_name) {
    if (!group_name) {
        return false;
    }
    return find_group(group_name) != NULL;
}

int softbus_api_get_group_devices(const char* group_name, char** device_names, int* count) {
    if (!group_name || !device_names || !count || *count <= 0) {
        return SOFTBUS_INVALID_ARG;
    }

    group_manager_t* group = find_group(group_name);
    if (!group) {
        return SOFTBUS_NOT_FOUND;
    }

    int copy_count = (*count < group->member_count) ? *count : group->member_count;
    
    for (int i = 0; i < copy_count; i++) {
        strncpy(device_names[i], group->members[i], MAX_NAME_LENGTH - 1);
        device_names[i][MAX_NAME_LENGTH - 1] = '\0';
    }

    *count = copy_count;
    return SOFTBUS_OK;
}

// 内部函数实现
static device_manager_t* find_device(const char* device_name) {
    if (!device_name) {
        return NULL;
    }
    return device_manager_find(device_name);
}

static group_manager_t* find_group(const char* group_name) {
    if (!group_name) {
        return NULL;
    }
    
    for (int i = 0; i < g_group_count; i++) {
        if (strcmp(g_groups[i].name, group_name) == 0) {
            return &g_groups[i];
        }
    }
    return NULL;
}

// 添加外部处理函数的声明
extern int temperature_sensor_handler(const char* msg, message_type_t type);
extern int led_controller_handler(const char* msg, message_type_t type); 