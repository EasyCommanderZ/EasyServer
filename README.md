# EasyServer

![GitHub](https://img.shields.io/github/license/EasyCommanderZ/EasyServer?style=for-the-badge)

EasyServer 是使用 C++ 17 标准开发的轻量级Web服务器。

## 项目特点

- 基于 C++ 17 标准开发，使用标准库提供的智能指针或将类按照RAII方式来设计，降低内存泄漏的可能。
- 使用 线程池 + 非阻塞 Socket + Epoll (ET) + Reactor模式的并发模型。主线程作为 Acceptor 接受请求，并分发给其他事件处理线程。
- 使用基于标准库优先队列 (Priority_Queue) 实现的定时器关闭超时请求，采用惰性删除。
- 使用 eventfd 实现了线程的异步唤醒。
- 实现了一个简单的异步日志模块，使用链式缓冲。
- 使用状态机 + 正则表达式解析 HTTP 请求。

## 环境

仅列出我的开发环境以供参考。

- OS : Debian 4.19.146-1 (2020-09-17) x86_64 GNU/Linux ( Debian 10.12 )
- Compiler : GCC 8.3.0 x86_64-linux-gnu
- CMake : 3.13.4 ( set minimum required version to 3.12 in CMakeLists ) 

由于使用了 Epoll，目前仅能在符合工具版本条件的 Linux 环境上进行完整编译。Log 部分在 macOS上开发，即 Log 部分确保可以在符合条件的 macOS 环境上编译运行。下面列出我在 macOS 的开发环境以供参考。

- OS : macOS Monterey 12.4, Apple M1
- Compiler : Clang 13.1.6 arm64-apple-darwin 21.4.0
- CMake : 3.23.2

未来期望完成 EasyServer 的跨平台实现。

## 编译

```bash
./build.sh
```

或者使用CMake：

```bash
cmake -B build
cd build
cmake --build .
```

推荐使用 VSCode 的 CMake Tools 拓展。

## 测试

测试部分主要分为功能测试和性能测试。

**功能测试**

**性能测试**

## 关于文档

在 [docs](./docs) 中，记录了项目中模块设计与实现的一些思路、问题和细节，也有一些项目过程中的学习、感受和思考，希望对于和我一样的学习者有所帮助。

## 致谢

参考的项目有很多，记录在 [开始Coding之前](./docs/BeforeStart.md) 的最后。非常感谢这些无私的开源作者！