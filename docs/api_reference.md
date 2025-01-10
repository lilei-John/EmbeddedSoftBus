# 软总线API参考手册

版本：1.0.0
最后更新：2024-01-10

## 概述
软总线API提供了一套完整的分布式系统设备通信和管理接口。它支持设备注册、消息传递、组管理和资源分配，着重强调可靠性和效率。

## 目录
- [数据类型](#数据类型)
- [初始化与清理](#初始化与清理)
- [设备管理](#设备管理)
- [消息管理](#消息管理)
- [组管理](#组管理)
- [资源管理](#资源管理)
- [错误码](#错误码)

## 数据类型

### 设备类型 (device_type_t)
```c
typedef enum {
    DEVICE_TYPE_LED,              // LED设备
    DEVICE_TYPE_TEMPERATURE_SENSOR, // 温度传感器
    DEVICE_TYPE_MAX               // 设备类型数量
} device_type_t;
```

### 消息类型 (message_type_t)
```c
typedef enum {
    MSG_TYPE_CONTROL,     // 控制消息（如：开关控制）
    MSG_TYPE_STATUS,      // 状态消息（如：状态查询）
    MSG_TYPE_DATA,        // 数据消息（如：传感器读数）
    MSG_TYPE_MAX         // 消息类型数量
} message_type_t;
```

### 消息优先级 (softbus_priority_t)
```c
typedef enum {
    SOFTBUS_PRIO_LOW,     // 低优先级（后台任务）
    SOFTBUS_PRIO_NORMAL,  // 普通优先级（常规操作）
    SOFTBUS_PRIO_HIGH,    // 高优先级（重要操作）
    SOFTBUS_PRIO_URGENT   // 紧急优先级（关键操作）
} softbus_priority_t;
```

## 初始化与清理

### softbus_api_init
```c
int softbus_api_init(void);
```
**功能**：初始化软总线系统
**参数**：无
**返回值**：
- `SOFTBUS_OK`：初始化成功
- `SOFTBUS_ERROR`：初始化失败
**说明**：必须在使用其他API之前调用

### softbus_api_deinit
```c
void softbus_api_deinit(void);
```
**功能**：清理软总线系统
**参数**：无
**返回值**：无
**说明**：在程序结束前调用，释放所有资源

## 设备管理

### softbus_api_register_device
```c
int softbus_api_register_device(device_type_t type, const char* device_name);
```
**功能**：注册设备到软总线系统
**参数**：
- `type`：设备类型，参见device_type_t
- `device_name`：设备名称，最大长度32字符
**返回值**：
- `SOFTBUS_OK`：注册成功
- `SOFTBUS_ERROR`：注册失败
**说明**：设备名称必须唯一

### softbus_api_unregister_device
```c
int softbus_api_unregister_device(const char* device_name);
```
**功能**：从软总线系统注销设备
**参数**：
- `device_name`：设备名称
**返回值**：
- `SOFTBUS_OK`：注销成功
- `SOFTBUS_ERROR`：注销失败
**说明**：会清理设备相关的所有资源

## 消息管理

### softbus_api_send_message
```c
int softbus_api_send_message(const char* target, message_type_t type, 
                           const char* message, softbus_priority_t priority);
```
**功能**：发送消息到指定设备
**参数**：
- `target`：目标设备名称
- `type`：消息类型
- `message`：消息内容
- `priority`：消息优先级
**返回值**：
- `SOFTBUS_OK`：发送成功
- `SOFTBUS_ERROR`：发送失败
**说明**：消息内容最大长度1024字节

### softbus_api_send_group_message
```c
int softbus_api_send_group_message(const char* group_name, message_type_t type,
                                 const char* message, softbus_priority_t priority);
```
**功能**：发送消息到设备组
**参数**：
- `group_name`：组名称
- `type`：消息类型
- `message`：消息内容
- `priority`：消息优先级
**返回值**：
- `SOFTBUS_OK`：发送成功
- `SOFTBUS_ERROR`：发送失败
**说明**：消息会发送给组内所有设备

### softbus_api_process_device_messages
```c
int softbus_api_process_device_messages(const char* device_name);
```
**功能**：处理设备的消息队列
**参数**：
- `device_name`：设备名称
**返回值**：
- `SOFTBUS_OK`：处理成功
- `SOFTBUS_ERROR`：处理失败
**说明**：按优先级顺序处理消息

## 组管理

### softbus_api_create_device_group
```c
int softbus_api_create_device_group(const char* group_name);
```
**功能**：创建设备组
**参数**：
- `group_name`：组名称，最大长度32字符
**返回值**：
- `SOFTBUS_OK`：创建成功
- `SOFTBUS_ERROR`：创建失败
**说明**：组名称必须唯一

### softbus_api_add_device_to_group
```c
int softbus_api_add_device_to_group(const char* group_name, const char* device_name);
```
**功能**：添加设备到组
**参数**：
- `group_name`：组名称
- `device_name`：设备名称
**返回值**：
- `SOFTBUS_OK`：添加成功
- `SOFTBUS_ERROR`：添加失败
**说明**：一个设备可以属于多个组

### softbus_api_remove_device_from_group
```c
int softbus_api_remove_device_from_group(const char* group_name, const char* device_name);
```
**功能**：从组中移除设备
**参数**：
- `group_name`：组名称
- `device_name`：设备名称
**返回值**：
- `SOFTBUS_OK`：移除成功
- `SOFTBUS_ERROR`：移除失败

### softbus_api_delete_device_group
```c
int softbus_api_delete_device_group(const char* group_name);
```
**功能**：删除设备组
**参数**：
- `group_name`：组名称
**返回值**：
- `SOFTBUS_OK`：删除成功
- `SOFTBUS_ERROR`：删除失败
**说明**：会自动清理组内所有设备的组关系

## 资源管理

### softbus_api_request_resource
```c
void* softbus_api_request_resource(const char* device_name, const char* resource_name);
```
**功能**：请求设备资源
**参数**：
- `device_name`：设备名称
- `resource_name`：资源名称
**返回值**：
- 非NULL：资源指针
- NULL：请求失败
**说明**：返回的资源指针类型需要根据资源类型进行转换

### softbus_api_release_resource
```c
int softbus_api_release_resource(const char* device_name, const char* resource_name);
```
**功能**：释放设备资源
**参数**：
- `device_name`：设备名称
- `resource_name`：资源名称
**返回值**：
- `SOFTBUS_OK`：释放成功
- `SOFTBUS_ERROR`：释放失败

## 错误码

### SOFTBUS_OK
```c
#define SOFTBUS_OK    0
```
**说明**：操作成功

### SOFTBUS_ERROR
```c
#define SOFTBUS_ERROR (-1)
```
**说明**：操作失败

## 使用限制

1. 名称长度限制
   - 设备名称：最大32字符
   - 组名称：最大32字符
   - 资源名称：最大32字符

2. 容量限制
   - 每个组最多32个设备
   - 消息内容最大1024字节
   - 设备类型数量不超过DEVICE_TYPE_MAX

3. 线程安全
   - 所有API都是线程安全的
   - 资源访问是互斥的

4. 内存使用
   - 消息结构使用固定大小内存
   - 不使用动态内存分配
   - 避免内存碎片 