#ifndef __SRC_POLLER_POLLER_H_
#define __SRC_POLLER_POLLER_H_

#include <memory>
#include <sys/epoll.h>
#include <vector>
#include "Channel.h"
#include "HttpData.h"
#include "Poller/Timer.h"
class Poller {
public:
    using SP_Channel = std::shared_ptr<Channel>;
    
    void epoll_add(SP_Channel request, int timeout);
    void epoll_mod(SP_Channel request, int timeout);
    void epoll_del(SP_Channel request, int timeout);
    int getPollerFd() {
        return _pollerFD;
    }
    std::vector<SP_Channel> poll();
private:
    static const int MAXFDS = 65535;
    int _pollerFD;
    std::vector<epoll_event> _events;
    SP_Channel _fdToChannel[MAXFDS];
    std::shared_ptr<HttpData> _fdToHttp;
    TimerManager _timerManager;
};

#endif /* __SRC_POLLER_POLLER_H_ */
