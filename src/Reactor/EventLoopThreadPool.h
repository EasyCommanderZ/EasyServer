#ifndef __SRC_POLLER_EVENTLOOPTHREADPOOL_H_
#define __SRC_POLLER_EVENTLOOPTHREADPOOL_H_

#include "Http/HttpData.h"
#include "Log/Logger.h"
#include "Reactor/EventLoopThread.h"
#include "Util/noncopyable.h"
#include <memory>
class EventLoopThreadPool : noncopyable {
private:
    EventLoop *_baseLoop;
    bool _started;
    int _numThreads;
    int _next;
    std::vector<std::shared_ptr<EventLoopThread>> _threads;
    std::vector<EventLoop *> _loops;

public:
    EventLoopThreadPool(EventLoop *baseLoop, int numThreads);
    ~EventLoopThreadPool() {
        LOG_INFO("EventLoopThreadPool exit \n", 0);
    };
    void start();
    EventLoop *getNextLoop();
};

#endif /* __SRC_POLLER_EVENTLOOPTHREADPOOL_H_ */
