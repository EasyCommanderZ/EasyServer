#ifndef __SRC_THREADPOOL_THREADPOOL_H_
#define __SRC_THREADPOOL_THREADPOOL_H_

#include "MTQueue.h"
#include "Thread/threadGroup.h"
#include <atomic>
#include <functional>
#include <sys/_types/_size_t.h>
#include <thread>

const int MAX_THREADS = 1024;

class ThreadPool {
public:
    using task_t = std::function<void()>;

    void InitializeWorkers(size_t workers_num);                                    // 初始化线程池
    explicit ThreadPool(size_t workers_num = std::thread::hardware_concurrency()); // 规定构造函数为显式，线程数默认值设置为 std::thread::hardware_concurrency()
    void addTask(task_t t);                                                        // 添加任务
    void stop();                                                                   // 关闭线程池
    ~ThreadPool();                                                                 // 在析构函数中要等待线程执行的结束

private:
    std::atomic_bool _running{true};   // 运行标记
    MTQueue<task_t> _task_queue;       // 任务队列
    std::vector<std::thread> _workers; // 线程数组
    // may use threadGroup in future version
    // threadGroup _workers;
};

#endif /* __SRC_THREADPOOL_THREADPOOL_H_ */
