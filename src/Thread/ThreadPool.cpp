#include "ThreadPool.h"
#include "Thread/threadGroup.h"
#include <cstddef>
#include <functional>
#include <thread>

using task_t = std::function<void()>;

void ThreadPool::InitializeWorkers(size_t workers_num) {
    for (size_t i = 0; i < workers_num; i++) {
        std::thread t([&]() {
            while (this->_running) {
                auto task = this->_task_queue.pop();
                if (task) task();
            }
        });
        this->_workers.push_back(std::move(t));
    }
}

ThreadPool::ThreadPool(size_t workers_num) {
    InitializeWorkers(workers_num);
}

void ThreadPool::stop() {
    this->_running = false;

    for (size_t i = 0; i < this->_workers.size(); i++) {
        this->_task_queue.push_back(nullptr); // 防止线程在没有任务的情况下关闭的时候会阻塞在获取task的地方
    }

    for (auto &w : _workers) {
        w.join();
    }
}

ThreadPool::~ThreadPool() {
    if (this->_running) {
        stop();
    }
}