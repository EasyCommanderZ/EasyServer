# Log 日志模块

Log 模块的实现稍微有一些复杂，我参考了一些项目，仿照 muduo 的日志进行日志库的实现。实现思路大致如下：

1. 首先，最基本的，日志需要一个写文件模块作为最底层的文件读写，并向外提供像指定文件名写入内容的接口；
2. 对上述的文件写入类进行封装，编写一个日志的文件类。需要对日志的文件名进行定制化，在此基础上对日志文件进行切分；
3. 封装不同的日志类型，同步日志和异步日志；
4. 封装一个对外使用的接口类，并且这个类可以自己选择日志的类型（同步和异步）；

按照这个大致的思路，在日志模块中实现了如下类：

1. FileWriter.h && FileWriter.cpp : 基本的写文件类。实现了文件写，刷盘等函数。在构造函数中获取文件指针，在析构函数中关闭文件，符合RAII。
2. LogFile.h && LogFile.cpp：基于FileWriter类，增加了定制化的日志文件名，并且增加了对日志的切分功能，按照配置的大小对日志进行文件切分。管理FileWriter指针使用unique_ptr，便于切分时文件指针的管理。
3. LogBuffer.h：封装的日志缓冲区类，包含一个c-style的字符缓冲区，一个标识当前缓冲区使用大小的变量。提供一些便捷使用的接口，如查询缓冲区内容，缓冲区当前大小，当前起始位置，清空等。
4. LogConfig.h：封装日志所需要的一些基本配置，并且提供默认值以及一个默认配置对象。
5. SynLog.h && SynLog.cpp：同步日志的类，阻塞式的写入日志。
6. CountDownLatch.h && CountDownLatch.cpp：一个锁存器，确保异步日志的工作线程启动之后，启动函数再返回。
7. AsynLog.h && AsynLog.cpp：异步日志的类。实现了一个基于链表的缓冲队列，并且能够自动生成和归还缓冲区。使用condition_variable来实现一个生产者-消费者的模型，达到异步写入的效果。
8. Logger.h && Logger.cpp：Log module的接口类，使用单例模式提供日志实例， 通过不同的outputFunc和flushFunc来设置日志的输出方式，通过LogConfig进行日志的配置，格式化时间来进一步定制文件名。使用几个宏来提供向外的日志接口。

---

总结：

Log的实现是参考muduo的模式：分为前端和后端，前端往后端写，后端往文件写。前端负责生产日志，后端通过多个缓冲区，收集前端生产的日志，往文件中写。

---

## 一些问题

Log 的实现分为前端和后端，前端往后端写，后端往磁盘写。为什么要区分前后端？主要的原因还是，涉及到IO操作，需要想办法让他的运行能够更快。

Log前端是IO线程，负责产生Log，后端是写日志线程，设计了多个缓冲区，负责收集前端产生的Log，然后往磁盘中写入。这样，Log的生产并输送到后端的速度是很快的，偏慢的写入操作就交给后端去执行。

异步日志中，后端是由多个缓冲区构成的，缓冲区满的时候或者刷盘时间到了就向文件中写入一次。后端的缓冲区有两组，第一组是放接收到的生产出来的Log的，另外一组是放写入文件的日志的。第一组的缓冲会自动增长，第二组的缓冲区在写入文件成功后会清空内容归还到第一组中。

---

总的来说，日志模块中类的封装还是比较复杂的，但是这样的结构也可以提供更强的拓展性。这样的实现主要还是参照了muduo的日志库实现，大部分都是在学习它的实现细节。代码里有许多问题都需要边实现才能感受到它的设计思路，发现实现中的一些细节，获益良多。