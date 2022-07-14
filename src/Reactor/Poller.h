#ifndef __SRC_REACTOR_POLLER_H_
#define __SRC_REACTOR_POLLER_H_

#include <unordered_map>
#if defined(__linux__) || defined(__linux)
#include <sys/epoll.h>
#endif

// #if defined (__linux__) || defined (__linux)

// #endif

#include <memory>
#include <vector>
#include "Channel.h"
#include "../Http/HttpData.h"
#include "Timer.h"
class Poller {
public:
    using SP_Channel = std::shared_ptr<Channel>;
    Poller();
    ~Poller();
    void poller_add(SP_Channel request, int timeout);
    void poller_mod(SP_Channel request, int timeout);
    void poller_del(SP_Channel request);
    std::vector<SP_Channel> poll();                          // 返回活跃事件
    std::vector<SP_Channel> getEventRequest(int events_num); // 分发处理函数
    void add_timer(SP_Channel request_data, int timeout);
    int getPollerFd() {
        return _pollerFD;
    }
    void handleExpired();

private:
    static const int MAXFDS = 100000;
    int _pollerFD;
#if defined(__linux__) || defined(__linux)
    std::vector<epoll_event> _events;
#endif
    SP_Channel _fdToChannel[MAXFDS];
    // std::unordered_map<int, SP_Channel> _fdToChannel;
    std::shared_ptr<HttpData> _fdToHttp[MAXFDS];
    // std::unordered_map<int , std::shared_ptr<HttpData>> _fdToHttp;
    TimerManager _timerManager;
};

#endif /* __SRC_REACTOR_POLLER_H_ */
