# Thread 模块

本来这个部分是 ThreadPool，但是在多参考了几个项目之后，发现似乎并不能概括这里的功能。For future development，还是修改成了 Thread。

目前，Thread中只实现了一个简单的线程池，其功能包括：

1. 在线程池中初始化指定数量的线程
2. 向任务队列添加任务 / 任务队列中的任务被空闲的线程消费
3. 关闭线程池，并等待已有的线程结束

其中的文件包括：

1. MTQueue.h：封装了一个线程安全的任务队列，基于 condition_variable， unique_lock 和queue。没有做队列大小的限制。
2. ThreadPool.h && ThreadPool.cpp : 线程池的实现。线程使用vector存储，最大线程数限制为1024；任务队列基于上面封装的 MTQueue。
3. ~~threadGroup.h : 待开发~~

当前线程池的实现比较简单， 功能之间也没有什么复杂的嵌套，就不多写什么了。只要对多线程编程有所了解就可以看懂。

这里再次推荐B站小彭老师的课程。https://www.bilibili.com/video/BV1fa411r7zp?vd_source=209053787962bd3d015372920f579b92

---

