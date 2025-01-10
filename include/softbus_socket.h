#ifndef SOFTBUS_SOCKET_H
#define SOFTBUS_SOCKET_H

#include "softbus_types.h"

// 是否启用socket组播功能
#define ENABLE_SOCKET_MULTICAST 1

#if ENABLE_SOCKET_MULTICAST

// Socket组播配置
#define MULTICAST_PORT 45678
#define MULTICAST_GROUP "239.0.0.1"
#define MAX_MSG_SIZE 1024

// Socket组播初始化
int socket_multicast_init(void);

// Socket组播清理
void socket_multicast_deinit(void);

// 发送组播消息
int socket_multicast_send(const char* message, size_t len);

// 接收组播消息
int socket_multicast_receive(char* buffer, size_t buffer_size, int timeout_ms);

// 启动组播接收线程
int socket_multicast_start_receiver(void);

// 停止组播接收线程
void socket_multicast_stop_receiver(void);

#endif // ENABLE_SOCKET_MULTICAST

#endif // SOFTBUS_SOCKET_H 