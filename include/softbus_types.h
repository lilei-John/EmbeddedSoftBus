#ifndef SOFTBUS_TYPES_H
#define SOFTBUS_TYPES_H

// 返回值定义
#define SOFTBUS_OK           0
#define SOFTBUS_ERROR       -1
#define SOFTBUS_INVALID_ARG -2
#define SOFTBUS_NOT_FOUND   -3
#define SOFTBUS_BUSY        -4
#define SOFTBUS_TIMEOUT     -5
#define SOFTBUS_NO_MEM      -6

// 设备类型枚举
typedef enum {
    DEVICE_TYPE_SENSOR,
    DEVICE_TYPE_ACTUATOR,
    DEVICE_TYPE_CONTROLLER,
    DEVICE_TYPE_DISPLAY,
    DEVICE_TYPE_OTHER
} device_type_t;

// 消息类型枚举
typedef enum {
    MESSAGE_TYPE_COMMAND,
    MESSAGE_TYPE_DATA,
    MESSAGE_TYPE_STATUS,
    MESSAGE_TYPE_RESPONSE,
    MESSAGE_TYPE_ERROR
} message_type_t;

// 优先级枚举
typedef enum {
    PRIORITY_LOW,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_URGENT
} softbus_priority_t;

// 消息广播模式
typedef enum {
    SOFTBUS_UNICAST,
    SOFTBUS_MULTICAST,
    SOFTBUS_BROADCAST
} softbus_cast_mode_t;

// 消息发送模式
typedef enum {
    SOFTBUS_MODE_ASYNC,  // 异步模式：发送后立即返回
    SOFTBUS_MODE_SYNC    // 同步模式：等待接收方处理完成后返回
} softbus_mode_t;

// 同步等待超时时间（毫秒）
#define SOFTBUS_SYNC_TIMEOUT_DEFAULT 5000

#endif // SOFTBUS_TYPES_H 