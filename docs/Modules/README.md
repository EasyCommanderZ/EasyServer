# /Docs/Classes

记录关于项目中代码文件的划分。按照功能初步划分如下：

1. log：server中的日志模块
2. http：处理HTTP请求
3. server：使用epoll处理socket连接
4. thread：多线程相关的实现，如线程池等
5. addons：额外功能
6. test：功能测试和性能测试



先暂时按照如上划分。在WebServer中还有一些地方需要封装一个Timer来处理不活动的连接，需要封装buffer来实现一个功能更完善的缓冲区。虽然作为server的优化功能，添加的时候也与server平级，然后作为引用引入。

目前对于实现顺序的想法，根据相互之间引用关系，thread是最基本的，server中的epoll以及server相关函数的实现都需要借助thread，另外，要实现多线程的异步日志模块，也需要使用到thread，所以首先实现thread。

http是相对独立的，它的功能是提供对网络包的校验，解析和构建，只需要提供接口给server即可，本身可以不涉及到其他的功能。

log也是如此，除了需要thread以外，功能和WebServer的其他部分相对独立。

所以，实现顺序应当首先实现thread，然后实现server，之后实现http以及log。先把这些核心功能实现，并且实现相应的测试。

