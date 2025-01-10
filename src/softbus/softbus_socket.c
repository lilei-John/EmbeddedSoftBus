#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#define close closesocket
#define sleep(x) Sleep((x)*1000)
#define usleep(x) Sleep((x)/1000)
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <pthread.h>
#include <errno.h>
#include "softbus_socket.h"
#include "softbus_types.h"
#include "message_queue.h"

#if ENABLE_SOCKET_MULTICAST

static int g_socket_fd = -1;
static struct sockaddr_in g_multicast_addr;
static volatile int g_receiver_running = 0;
static pthread_t g_receiver_thread;

// 组播接收线程函数
static void* multicast_receiver_thread(void* arg) {
    char buffer[MAX_MSG_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    while (g_receiver_running) {
        // 接收组播消息
        ssize_t recv_len = recvfrom(g_socket_fd, buffer, sizeof(buffer) - 1, 0,
                                  (struct sockaddr*)&sender_addr, &sender_addr_len);
        
        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            printf("Received multicast message: %s\n", buffer);

            // 创建消息并发送到消息队列
            message_t msg = {0};
            strncpy(msg.target, "multicast", sizeof(msg.target) - 1);
            msg.type = MESSAGE_TYPE_COMMAND;
            msg.priority = PRIORITY_NORMAL;
            strncpy(msg.content, buffer, sizeof(msg.content) - 1);
            message_queue_send(&msg);
        }
    }

    return NULL;
}

int socket_multicast_init(void) {
    // 创建UDP socket
    g_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_socket_fd < 0) {
        perror("Failed to create socket");
        return SOFTBUS_ERROR;
    }

    // 设置地址重用
    int reuse = 1;
    if (setsockopt(g_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Failed to set SO_REUSEADDR");
        close(g_socket_fd);
        return SOFTBUS_ERROR;
    }

    // 设置组播地址
    memset(&g_multicast_addr, 0, sizeof(g_multicast_addr));
    g_multicast_addr.sin_family = AF_INET;
    g_multicast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    g_multicast_addr.sin_port = htons(MULTICAST_PORT);

    // 绑定socket
    if (bind(g_socket_fd, (struct sockaddr*)&g_multicast_addr, sizeof(g_multicast_addr)) < 0) {
        perror("Failed to bind socket");
        close(g_socket_fd);
        return SOFTBUS_ERROR;
    }

    // 加入组播组
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(g_socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Failed to join multicast group");
        close(g_socket_fd);
        return SOFTBUS_ERROR;
    }

    return SOFTBUS_OK;
}

void socket_multicast_deinit(void) {
    if (g_socket_fd >= 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
    }
}

int socket_multicast_send(const char* message, size_t len) {
    if (!message || len == 0 || len > MAX_MSG_SIZE) {
        return SOFTBUS_INVALID_ARG;
    }

    // 设置目标地址
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    dest_addr.sin_port = htons(MULTICAST_PORT);

    // 发送消息
    ssize_t sent_len = sendto(g_socket_fd, message, len, 0,
                             (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (sent_len < 0) {
        perror("Failed to send multicast message");
        return SOFTBUS_ERROR;
    }

    return SOFTBUS_OK;
}

int socket_multicast_receive(char* buffer, size_t buffer_size, int timeout_ms) {
    if (!buffer || buffer_size == 0) {
        return SOFTBUS_INVALID_ARG;
    }

    // 设置接收超时
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(g_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Failed to set receive timeout");
        return SOFTBUS_ERROR;
    }

    // 接收消息
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    ssize_t recv_len = recvfrom(g_socket_fd, buffer, buffer_size - 1, 0,
                               (struct sockaddr*)&sender_addr, &sender_addr_len);

    if (recv_len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return SOFTBUS_TIMEOUT;
        }
        perror("Failed to receive multicast message");
        return SOFTBUS_ERROR;
    }

    buffer[recv_len] = '\0';
    return SOFTBUS_OK;
}

int socket_multicast_start_receiver(void) {
    if (g_receiver_running) {
        return SOFTBUS_OK;  // 已经在运行
    }

    g_receiver_running = 1;
    if (pthread_create(&g_receiver_thread, NULL, multicast_receiver_thread, NULL) != 0) {
        perror("Failed to create receiver thread");
        g_receiver_running = 0;
        return SOFTBUS_ERROR;
    }

    return SOFTBUS_OK;
}

void socket_multicast_stop_receiver(void) {
    if (g_receiver_running) {
        g_receiver_running = 0;
        pthread_join(g_receiver_thread, NULL);
    }
}

#endif // ENABLE_SOCKET_MULTICAST 