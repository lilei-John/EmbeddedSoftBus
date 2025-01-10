# 嵌入式软总线系统

[English Version](README.md)

一个轻量级、高效且灵活的嵌入式系统软总线实现，为设备通信和资源管理提供统一的抽象层。

## 功能特性

### 核心功能
- 基于消息的通信机制
- 基于优先级的消息处理
- 单播和组播消息支持
- 设备注册和管理
- 资源请求和管理
- 基于分组的设备组织

### 技术特性
- 基于红黑树的消息队列
- 线程安全操作
- 高效的消息路由
- 最小内存占用
- 无硬件依赖性

## 快速开始

### 环境要求
- C编译器（推荐使用GCC）
- Make构建系统
- Windows或Linux操作系统

### 构建项目
```bash
# 清理之前的构建
mingw32-make clean

# 构建项目
mingw32-make
```

### 简单示例
```c
#include "softbus_api.h"

int main() {
    // 初始化
    softbus_api_init();

    // 注册设备
    softbus_api_register_device(DEVICE_TYPE_LED, "led1");

    // 发送消息
    softbus_api_send_message("led1", MSG_TYPE_CONTROL, 
                           "打开", SOFTBUS_PRIO_NORMAL);

    // 处理消息
    softbus_api_process_device_messages("led1");

    // 清理
    softbus_api_unregister_device("led1");
    softbus_api_deinit();
    return 0;
}
```

## 文档

### 用户指南
- [快速入门指南](docs/quickstart.md)
- [软总线系统手册](docs/softbus_manual.md)
- [API参考文档](docs/api_reference.md)

### API文档
详细的API文档请参见[API参考文档](docs/api_reference.md)。

## 项目结构
```
.
├── include/
│   ├── softbus.h        # 核心接口
│   ├── softbus_api.h    # 高级API
│   └── rbtree.h         # 消息队列实现
├── src/
│   └── softbus/
│       ├── softbus.c    # 核心实现
│       ├── softbus_api.c # API实现
│       └── rbtree.c     # 红黑树实现
├── examples/
│   └── demo.c          # 使用示例
├── docs/
│   ├── quickstart.md   # 快速入门指南
│   └── softbus_manual.md # 详细手册
└── README.md           # 本文件
```

## 功能详解

### 消息优先级系统
- `SOFTBUS_PRIO_LOW`: 后台任务
- `SOFTBUS_PRIO_NORMAL`: 常规操作
- `SOFTBUS_PRIO_HIGH`: 重要操作
- `SOFTBUS_PRIO_URGENT`: 关键操作

### 消息类型
- `MSG_TYPE_CONTROL`: 设备控制命令
- `MSG_TYPE_STATUS`: 状态检查消息
- `MSG_TYPE_DATA`: 数据传输消息

### 设备管理
- 动态设备注册
- 设备类型抽象
- 资源管理
- 分组操作

## 性能特性

### 消息队列性能
- 消息插入：O(log n)
- 消息删除：O(log n)
- 基于优先级的检索：O(log n)

### 内存使用
- 固定大小的消息结构
- 高效的内存管理
- 无动态内存碎片

## 使用限制
- 每组最多32个设备
- 最大消息大小：1024字节
- 最大名称长度：32字符

## 参与贡献
1. Fork本仓库
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

## 许可证
本项目采用MIT许可证 - 详见LICENSE文件

## 致谢
- 红黑树实现参考了Linux内核
- 消息优先级系统基于RTOS概念
- 设备管理模式来自嵌入式系统最佳实践 