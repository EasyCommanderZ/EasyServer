# Reactor

EasyServer 的响应模型是 Reactor 模型，是一个以事件为驱动的IO多路复用处理模型。

Reactor 中包含了几个类：

1. Channel：Channel即为模型中的事件类。一个Channel只属于一个loop，负责一个文件描述符的IO事件，通过给 Channel 类设置不同的IO事件处理函数来实现不同的业务。当IO事件发生的时候，loop最终会调用 Channel 中绑定的回调函数。因此，服务器中需要进行读写处理的时间对象都会和一个 Channel 进行关联。监听socket中的listenFd，每个loop的wakeupFd，以及 http 事件的 HttpData 都是如此。
2. EventLoop：One loop per thread。一个工作线程中会有一个 EventLoop，他的作用是对poller中的活跃事件进行发现，并且分发到 Channel 中进行处理。这里所说的分发其实指的是针对每个 Channel 调用其绑定的事件处理函数，而不关心 Channel 所关联的业务种类。
3. EventLoopThread：封装 EventLoop 到具体的工作线程中，并且确保只有在 loop 真正创建完毕的时候对象指针才被返回。
4. EventLoopThreadPool：工作线程池的封装，和mainLoop是关联的。提供了轮转式获取工作线程的方法。
5. Poller：IO多路复用方法的包装。目前提供 Epoll 添加、修改、删除关注的文件描述符的接口，以及获取活跃事件、给事件绑定定时器以及处理过期事件的方法。
6. Timer：事件定时器以及定时器管理类的实现。使用优先队列进行实现，调用方式为惰性删除。
