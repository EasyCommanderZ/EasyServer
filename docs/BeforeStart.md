# 开始Coding之前

在开始写之前，有些问题需要理清楚。对于项目的一些想法还需要明确，有些问题需要问问自己。



### 我想要的是什么？

一个基于 C++ 11 & Linux 的高性能 Web 服务器。首先，它需要支持基本的功能有：

1. 客户端和服务端的正确连接
2. 基本的 HTTP 请求处理能力
3. 有较高的并发处理能力，符合 Web 服务器的应用场景
4. 可定制化的启动参数
5. 准确且高效而稳定的日志功能
6. 对于已有的设计，有配套的 test suite
7. 使用 CMAKE 进行项目的管理

上面是对于 EasyServer 的基本功能的要求。那么，基于这些，我还想要有：

1. 有连接 MySQL之类数据库的能力（当然，这一点应该通过业务逻辑而非 server 来实现）
2. 有点播传输/实时传输流媒体的能力（视频网站/直播网站）
3. 对于 2 中的功能，支持 multi-protocol
4. 跨平台能力
5. And more...



### 我需要什么

1. C++ 11 以上标准 C++ 的使用
2. CGI 校验
3. 多线程/多进程编程
4. Epoll
5. unit test, benchmark programming
6. CMAKE 对于复杂项目的管理
7. 音视频编码能力
8. And more...



### 关于项目

项目暂时就叫 EasyServer 吧，后续可以想一个更有意思的名字。



### 参考资料

有很多优秀的开源项目可以参考，稍微罗列一下用到的参考资料。

[TinyWebServer](https://github.com/qinguoyi/TinyWebServer) 以及它的 C++ 11 实现 [WebServer](https://github.com/markparticle/WebServer)

也是一个star很多的，参考muduo实现的 [WebServer](https://github.com/linyacool/WebServer)

一个看起来比较成熟的 C++ 11 网络框架 [ZLToolKit](https://github.com/ZLMediaKit/ZLToolKit) 以及作者基于这个框架开发的流媒体服务器 [ZLMediaKit](https://github.com/ZLMediaKit/ZLMediaKit)

[30dayMakeCppServer](https://github.com/yuesong-feng/30dayMakeCppServer) 虽然（截止目前）没有写完且已经有段时间没有更新了，已有的内容对项目帮助很大

蒋哥参照muduo实现的一个日志库 [TinyLog](https://github.com/CsJsss/TinyLog)

感谢以上这些无私的作者！