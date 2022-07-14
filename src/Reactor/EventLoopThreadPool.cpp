#include "EventLoopThreadPool.h"
#include "../Http/HttpData.h"
#include "../Log/Logger.h"
#include "EventLoopThread.h"
#include <cassert>
#include <cstdlib>
EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int numThreads) :
    _baseLoop(baseLoop), _numThreads(numThreads), _started(false), _next(0) {
    if (_numThreads <= 0) {
        LOG_ERROR("EventLoopThreadPool thread nums <= 0", 0);
        abort();
    }
}

void EventLoopThreadPool::start() {
    _baseLoop->assertInLoopThread();
    _started = true;
    for (int i = 0; i < _numThreads; i++) {
        std::shared_ptr<EventLoopThread> t(new EventLoopThread());
        _threads.push_back(t);
        _loops.push_back(t->startLoop());
    }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
    _baseLoop->assertInLoopThread();
    assert(_started);
    EventLoop *loop = _baseLoop;
    if (!_loops.empty()) {
        loop = _loops[_next];
        _next = (_next + 1) % _numThreads;
    }
    return loop;
}