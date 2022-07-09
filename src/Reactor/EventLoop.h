#ifndef __SRC_POLLER_EVENTLOOP_H_
#define __SRC_POLLER_EVENTLOOP_H_

#include "Poller.h"
#include <cassert>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include "Channel.h"
#include "../Util/sockUtil.h"

class EventLoop {
public:
    using Functor = std::function<void()>;
    using SP_Channel = std::shared_ptr<Channel>;

    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    void runInLoop(Functor&& func);
    void queueInLoop(Functor&& func);
    bool isInLoopThread() const {
        return _threadId == std::this_thread::get_id();
    }
    void assertInLoopThread() {
        assert(isInLoopThread());
    }
    void shutdown(SP_Channel channel) {
        shutDownWR(channel -> getFd());
    }
    void removeFromPoller(SP_Channel channel) {
        _poller -> poller_del(channel);
    }
    void updatePoller(SP_Channel channel, int timeout = 0) {
        _poller -> poller_mod(channel, timeout);
    }
    void addToPoller(SP_Channel channel, int timeout = 0) {
        _poller -> poller_add(channel, timeout);
    }

private:
    bool _looping;
    std::shared_ptr<Poller> _poller;
    int _wakeupFd;
    bool _quit;
    bool _eventHandling;
    mutable std::mutex _mtx;
    std::vector<Functor> _pendingFunctors;
    bool _callingPendingFunctors;
    const std::thread::id _threadId;
    SP_Channel _pwakeupChannel;

    void wakeup();
    void handleRead();
    void doPendingFunctors();
    void handleConn();
};

#endif /* __SRC_POLLER_EVENTLOOP_H_ */
