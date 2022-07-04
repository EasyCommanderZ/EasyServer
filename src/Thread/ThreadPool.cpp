#include "ThreadPool.h"
#include "Thread/threadGroup.h"
#include <cstddef>
#include <functional>
#include <stdexcept>
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
    if(workers_num > MAX_THREADS) workers_num = MAX_THREADS;
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

void ThreadPool::addTask(task_t t) {
    if(not this -> _running) {
        throw std::runtime_error("Thread pool addTask error : thread pool already stopped.");
    }
    this -> _task_queue.push_back(std::move(t));
}

ThreadPool::~ThreadPool() {
    if (this->_running) {
        stop();
    }
}