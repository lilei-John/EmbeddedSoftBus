#ifndef GROUP_MANAGER_H
#define GROUP_MANAGER_H

#include "softbus.h"

typedef struct {
    char name[32];
    char* devices[32];  // 组内设备列表
    int device_count;
    pthread_mutex_t mutex;
} device_group_t;

// 组管理API
int group_manager_init(void);
void group_manager_deinit(void);
int group_manager_create(const char* group_name);
int group_manager_delete(const char* group_name);
int group_manager_add_device(const char* group_name, const char* device_name);
int group_manager_remove_device(const char* group_name, const char* device_name);
device_group_t* group_manager_find(const char* group_name);

#endif // GROUP_MANAGER_H 