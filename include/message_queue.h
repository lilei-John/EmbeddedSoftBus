#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "message_types.h"
#include "softbus_types.h"

// 消息队列初始化
int message_queue_init(void);

// 消息队列清理
void message_queue_deinit(void);

// 发送消息
int message_queue_send(const message_t* msg);

// 接收消息
int message_queue_receive(const char* target, message_t* msg);

// 查看消息但不移除
int message_queue_peek(const char* target, message_t* msg);

// 设置消息完成回调
void message_queue_set_callback(const char* target, message_callback_t callback, void* user_data);

// 移除消息完成回调
void message_queue_remove_callback(const char* target);

#endif // MESSAGE_QUEUE_H 