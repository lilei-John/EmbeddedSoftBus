# Software Bus System

## 系统架构

软件总线系统由以下主要组件构成：

1. 设备管理器（Device Manager）
   - 管理设备注册和注销
   - 维护设备状态和信息
   - 提供设备查找功能

2. 消息队列（Message Queue）
   - 支持优先级消息
   - 线程安全的消息处理
   - 消息缓存和转发

3. 组管理器（Group Manager）
   - 管理设备组的创建和删除
   - 维护组成员关系
   - 支持组播消息

4. 资源管理器（Resource Manager）
   - 管理设备资源分配
   - 提供资源访问控制
   - 资源生命周期管理

## 使用说明

1. 初始化系统
```c
softbus_api_init();
```

2. 注册设备
```c
softbus_api_register_device(DEVICE_TYPE_LED, "my_led");
```

3. 发送消息
```c
softbus_api_send_message("my_led", MSG_TYPE_CONTROL, "ON", SOFTBUS_PRIO_NORMAL);
```

4. 创建和使用组
```c
softbus_api_create_group("light_group");
softbus_api_add_to_group("light_group", "my_led");
softbus_api_send_group_message("light_group", MSG_TYPE_CONTROL, "OFF", SOFTBUS_PRIO_NORMAL);
```

## 同步和异步模式

软件总线系统支持同步和异步两种消息发送模式：

1. 异步模式（默认）
```c
// 使用默认的异步API
softbus_api_send_message("my_led", MSG_TYPE_CONTROL, "ON", SOFTBUS_PRIO_NORMAL);

// 显式指定异步模式
softbus_api_send_message_ex("my_led", MSG_TYPE_CONTROL, "ON", 
                          SOFTBUS_PRIO_NORMAL, SOFTBUS_MODE_ASYNC, 0);
```

2. 同步模式
```c
// 使用同步模式，等待5秒超时
int ret = softbus_api_send_message_ex("my_led", MSG_TYPE_CONTROL, "ON",
                                    SOFTBUS_PRIO_NORMAL, SOFTBUS_MODE_SYNC, 5000);
if (ret == SOFTBUS_OK) {
    printf("消息已成功处理\n");
} else if (ret == SOFTBUS_TIMEOUT) {
    printf("等待超时\n");
} else {
    printf("发送失败: %d\n", ret);
}
```

3. 组消息的同步/异步发送
```c
// 异步发送组消息
softbus_api_send_group_message("light_group", MSG_TYPE_CONTROL, "OFF", SOFTBUS_PRIO_NORMAL);

// 同步发送组消息，等待所有设备处理完成
int ret = softbus_api_send_group_message_ex("light_group", MSG_TYPE_CONTROL, "OFF",
                                          SOFTBUS_PRIO_NORMAL, SOFTBUS_MODE_SYNC, 5000);
```

### 同步模式说明

- 同步模式下，发送函数会等待接收方处理完成后才返回
- 可以设置超时时间（毫秒），超时后返回 `SOFTBUS_TIMEOUT`
- 组消息的同步模式会等待组内所有设备都处理完成
- 默认超时时间为5000毫秒（5秒）

## 限制条件

- 最大设备数：32
- 最大组数：16
- 最大消息长度：1024字节
- 设备名最大长度：32字符 