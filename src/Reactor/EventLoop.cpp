
#include "../Log/Logger.h"
#include "Channel.h"
#include <cstdint>
#include <functional>
#include <mutex>
#include <sstream>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include "EventLoop.h"
#include "Poller.h"
#include "../Util/miscUtil.h"
#include "../Util/sockUtil.h"

thread_local EventLoop *t_loopInThisThread = nullptr;

int createEventFd() {
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_ERROR("Failed in eventFd\n", 0);
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop() :
    _looping(false), _poller(new Poller()), _wakeupFd(createEventFd()), _quit(false), _eventHandling(false), _callingPendingFunctors(false), _threadId(std::this_thread::get_id()), _pwakeupChannel(new Channel(this, _wakeupFd)) {
    if (t_loopInThisThread) {
        LOG_ERROR("EventLoop already exists! thread id : %s\n", tidToStr(_threadId));
    } else {
        t_loopInThisThread = this;
    }

    _pwakeupChannel->setEvents(EPOLLIN | EPOLLET);
    _pwakeupChannel->setReadHandler([this] { handleRead(); });
    _pwakeupChannel->setConnHandler([this] { handleConn(); });
    _poller->poller_add(_pwakeupChannel, 0);
}

EventLoop::~EventLoop() {
    close(_wakeupFd);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleConn() {
    updatePoller(_pwakeupChannel, 0);
}

void EventLoop::wakeup() {
    std::uint64_t one = 1;
    ssize_t n = writen(_wakeupFd, (char *)(&one), sizeof one);
    if (n != sizeof one) {
        LOG_INFO("EventLoop: wakeup() writes %d bytes instead of 8\n", n);
    }
}

void EventLoop::handleRead() {
    std::uint64_t one = 1;
    ssize_t n = readn(_wakeupFd, &one, sizeof one);
    if(n != sizeof one) {
        LOG_INFO("EventLoop: handleRead reads %d bytes instad of 8\n", n);
    }
    _pwakeupChannel -> setEvents(EPOLLIN | EPOLLET);
}

void EventLoop::queueInLoop(Functor &&func) {
    {
        std::unique_lock<std::mutex> lck(_mtx);
        _pendingFunctors.emplace_back(std::move(func));
    }
    if(!isInLoopThread() || _callingPendingFunctors) wakeup();
}

void EventLoop::runInLoop(Functor&& func) {
    if(isInLoopThread()) {
        func();
    } else {
        queueInLoop(std::move(func));
    }
}

void EventLoop::loop() {
    assert(!_looping);
    assert(isInLoopThread());
    _looping = true;
    _quit = false;
    LOG_TRACE("EventLoop starts looping, thread : %s\n", tidToStr(_threadId));
    std::vector<SP_Channel> ret;
    while(!_quit) {
        ret.clear();
        ret = _poller -> poll();
        _eventHandling = true;
        for(auto& ch : ret) ch -> handleEvents();
        _eventHandling = false;
        doPendingFunctors();
        _poller -> handleExpired();
    }
    _looping = false;
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> funcs;
    _callingPendingFunctors = true;
    {
        std::unique_lock<std::mutex> lck(_mtx);
        funcs.swap(_pendingFunctors);
    }
    for(size_t i = 0; i < funcs.size(); i ++) funcs[i]();
    _callingPendingFunctors = false;
}

void EventLoop::quit() {
    _quit = true;
    if(!isInLoopThread()) {
        wakeup();
    }
}