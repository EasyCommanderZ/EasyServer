#ifndef __SRC_POLLER_EVENTLOOPTHREAD_H_
#define __SRC_POLLER_EVENTLOOPTHREAD_H_

#include "EventLoop.h"
#include <condition_variable>
#include <mutex>
#include <thread>
class EventLoopThread {
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();
    EventLoop *_loop;
    bool _looping;
    bool _exiting;
    std::thread _thread;
    std::mutex _mtx;
    std::condition_variable _cv;
};

#endif /* __SRC_POLLER_EVENTLOOPTHREAD_H_ */
