#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "message_queue.h"
#include "device_manager.h"
#include "rbtree.h"
#include "softbus_types.h"
#include "message_types.h"
#include "softbus_internal.h"

// 消息回调结构体
typedef struct {
    char target[MAX_NAME_LENGTH];
    message_callback_t callback;
    void* user_data;
} message_callback_info_t;

// 全局变量
static pthread_mutex_t g_callback_mutex = PTHREAD_MUTEX_INITIALIZER;
static message_callback_info_t g_callbacks[MAX_DEVICES];
static int g_callback_count = 0;

// 内部函数声明
static message_callback_info_t* find_callback(const char* target);
static int insert_message(device_manager_t* dev, message_t* msg);

int message_queue_init(void) {
    pthread_mutex_init(&g_callback_mutex, NULL);
    g_callback_count = 0;
    memset(g_callbacks, 0, sizeof(g_callbacks));
    return 0;
}

void message_queue_deinit(void) {
    pthread_mutex_destroy(&g_callback_mutex);
}

int message_queue_send(const message_t* msg) {
    if (!msg || !msg->target) {
        return SOFTBUS_INVALID_ARG;
    }

    printf("Sending message to %s: type=%d, content=%s\n", 
           msg->target, msg->type, msg->content);

    // 查找目标设备
    device_manager_t* dev = device_manager_find(msg->target);
    if (!dev) {
        printf("Target device not found: %s\n", msg->target);
        return SOFTBUS_NOT_FOUND;
    }

    // 创建消息副本
    message_t* new_msg = (message_t*)malloc(sizeof(message_t));
    if (!new_msg) {
        printf("Failed to allocate memory for new message\n");
        return SOFTBUS_ERROR;
    }
    memcpy(new_msg, msg, sizeof(message_t));

    // 如果消息包含数据，复制数据
    if (msg->data && msg->data_len > 0) {
        new_msg->data = malloc(msg->data_len);
        if (!new_msg->data) {
            printf("Failed to allocate memory for message data\n");
            free(new_msg);
            return SOFTBUS_ERROR;
        }
        memcpy(new_msg->data, msg->data, msg->data_len);
    } else {
        new_msg->data = NULL;
        new_msg->data_len = 0;
    }

    // 设置消息时间戳
    clock_gettime(CLOCK_REALTIME, &new_msg->timestamp);

    // 插入消息到设备的消息树中
    int ret = insert_message(dev, new_msg);
    if (ret != SOFTBUS_OK) {
        printf("Failed to insert message into queue\n");
        if (new_msg->data) {
            free(new_msg->data);
        }
        free(new_msg);
        return ret;
    }

    printf("Message successfully queued for %s\n", msg->target);

    // 查找并调用回调函数
    message_callback_info_t* callback_info = find_callback(msg->target);
    if (callback_info && callback_info->callback) {
        printf("Calling message callback for %s\n", msg->target);
        callback_info->callback(msg->target, SOFTBUS_OK, callback_info->user_data);
    }

    return SOFTBUS_OK;
}

int message_queue_peek(const char* target, message_t* msg) {
    if (!target || !msg) {
        return SOFTBUS_INVALID_ARG;
    }

    // 查找目标设备
    device_manager_t* dev = device_manager_find(target);
    if (!dev) {
        return SOFTBUS_NOT_FOUND;
    }

    // 获取第一个消息
    struct rb_node* node = rb_first(&dev->msg_tree);
    if (!node) {
        return SOFTBUS_NOT_FOUND;
    }

    // 复制消息内容
    message_t* first_msg = rb_entry(node, message_t, node);
    memcpy(msg, first_msg, sizeof(message_t));

    return SOFTBUS_OK;
}

int message_queue_receive(const char* target, message_t* msg) {
    if (!target || !msg) {
        return SOFTBUS_INVALID_ARG;
    }

    printf("Receiving message for %s\n", target);

    // 查找目标设备
    device_manager_t* dev = device_manager_find(target);
    if (!dev) {
        printf("Target device not found: %s\n", target);
        return SOFTBUS_NOT_FOUND;
    }

    // 获取并移除第一个消息
    struct rb_node* node = rb_first(&dev->msg_tree);
    if (!node) {
        printf("No messages in queue for %s\n", target);
        return SOFTBUS_NOT_FOUND;
    }

    // 复制消息内容
    message_t* first_msg = rb_entry(node, message_t, node);
    memcpy(msg, first_msg, sizeof(message_t));

    // 如果消息包含数据，复制数据
    if (first_msg->data && first_msg->data_len > 0) {
        msg->data = malloc(first_msg->data_len);
        if (!msg->data) {
            printf("Failed to allocate memory for received message data\n");
            return SOFTBUS_ERROR;
        }
        memcpy(msg->data, first_msg->data, first_msg->data_len);
    } else {
        msg->data = NULL;
        msg->data_len = 0;
    }

    printf("Retrieved message from queue: type=%d, content=%s\n", 
           msg->type, msg->content);

    // 从树中移除消息
    rb_erase(node, &dev->msg_tree);
    if (first_msg->data) {
        free(first_msg->data);
    }
    free(first_msg);

    return SOFTBUS_OK;
}

void message_queue_set_callback(const char* target, message_callback_t callback, void* user_data) {
    if (!target) {
        return;
    }

    pthread_mutex_lock(&g_callback_mutex);

    // 查找现有回调
    message_callback_info_t* callback_info = find_callback(target);
    if (callback_info) {
        // 更新现有回调
        callback_info->callback = callback;
        callback_info->user_data = user_data;
    } else if (g_callback_count < MAX_DEVICES) {
        // 添加新回调
        strncpy(g_callbacks[g_callback_count].target, target, MAX_NAME_LENGTH - 1);
        g_callbacks[g_callback_count].target[MAX_NAME_LENGTH - 1] = '\0';
        g_callbacks[g_callback_count].callback = callback;
        g_callbacks[g_callback_count].user_data = user_data;
        g_callback_count++;
    }

    pthread_mutex_unlock(&g_callback_mutex);
}

void message_queue_remove_callback(const char* target) {
    if (!target) {
        return;
    }

    pthread_mutex_lock(&g_callback_mutex);

    // 查找回调
    int idx = -1;
    for (int i = 0; i < g_callback_count; i++) {
        if (strcmp(g_callbacks[i].target, target) == 0) {
            idx = i;
            break;
        }
    }

    if (idx != -1) {
        // 移动回调列表以填补空缺
        for (int i = idx; i < g_callback_count - 1; i++) {
            memcpy(&g_callbacks[i], &g_callbacks[i + 1], sizeof(message_callback_info_t));
        }
        g_callback_count--;
    }

    pthread_mutex_unlock(&g_callback_mutex);
}

// 内部函数实现
static message_callback_info_t* find_callback(const char* target) {
    for (int i = 0; i < g_callback_count; i++) {
        if (strcmp(g_callbacks[i].target, target) == 0) {
            return &g_callbacks[i];
        }
    }
    return NULL;
}

static int insert_message(device_manager_t* dev, message_t* msg) {
    struct rb_node** p = &dev->msg_tree.rb_node;
    struct rb_node* parent = NULL;
    message_t* entry;

    // 查找插入位置
    while (*p) {
        parent = *p;
        entry = rb_entry(parent, message_t, node);

        // 根据时间戳和优先级排序
        if (msg->priority > entry->priority) {
            p = &(*p)->rb_left;
        } else if (msg->priority < entry->priority) {
            p = &(*p)->rb_right;
        } else {
            if (msg->timestamp.tv_sec < entry->timestamp.tv_sec ||
                (msg->timestamp.tv_sec == entry->timestamp.tv_sec &&
                 msg->timestamp.tv_nsec < entry->timestamp.tv_nsec)) {
                p = &(*p)->rb_left;
            } else {
                p = &(*p)->rb_right;
            }
        }
    }

    // 插入新节点
    rb_link_node(&msg->node, parent, p);
    rb_insert_color(&msg->node, &dev->msg_tree);

    return SOFTBUS_OK;
} 