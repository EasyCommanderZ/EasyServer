#include "EventLoopThread.h"
#include "EventLoop.h"
#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

EventLoopThread::EventLoopThread() :
    _loop(nullptr), _exiting(false), _looping(false){};

EventLoop *EventLoopThread::startLoop() {
    assert(!_looping);
    _thread = std::thread([this] { threadFunc(); });
    {
        std::unique_lock<std::mutex> lck(_mtx);
        while(_loop == nullptr) _cv.wait(lck);
    }
    return _loop;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;

    {
        // PROBLEM : will this work ?
        std::unique_lock<std::mutex> lck(_mtx);
        _loop = &loop;
        _cv.notify_one();
    }

    loop.loop();
    // _loop = nullptr;
}

EventLoopThread::~EventLoopThread() {
    _exiting = true;
    if(_loop != nullptr) {
        _loop -> quit();
        _thread.join();
    }
}