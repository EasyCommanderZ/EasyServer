#include "Poller.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include "../Log/Logger.h"

#if defined(__linux__) || defined(__linux)
#include <sys/epoll.h>
#endif

const int EVENTSNUM = 4096;
const int POLLER_WAIT_TIME = 10000;

Poller::Poller() :
    _pollerFD(epoll_create1(EPOLL_CLOEXEC)), _events(EVENTSNUM) {
    assert(_pollerFD > 0);
}

Poller::~Poller() = default;

// 注册新描述符
void Poller::poller_add(SP_Channel request, int timeout) {
    int fd = request->getFd();
    if (timeout > 0) {
        add_timer(request, timeout);
        _fdToHttp[fd] = request->getHolder();
    }
#if defined(__linux__) || defined(__linux)
    struct epoll_event event;
    event.events = request->getEvents();
#endif

    request->equalAndUpdateEvents();
    _fdToChannel[fd] = request;

#if defined(__linux__) || defined(__linux)
    if (epoll_ctl(_pollerFD, EPOLL_CTL_ADD, fd, &event) < 0) {
        std::perror("epoll_add error");
        _fdToChannel[fd].reset();
    }
#endif
}

// 修改描述符状态
void Poller::poller_mod(SP_Channel request, int timeout) {
    if (timeout > 0) {
        add_timer(request, timeout);
    }
    int fd = request->getFd();
    if (!request->equalAndUpdateEvents()) {
#if defined(__linux__) || defined(__linux)
        struct epoll_event event;
        event.data.fd = fd;
        event.events = request->getEvents();
        if (epoll_ctl(_pollerFD, EPOLL_CTL_MOD, fd, &event) < 0) {
            std::perror("epoll_mod error");
            _fdToChannel[fd].reset();
        }
#endif
    }
}

// 从 poller 中删除描述符
void Poller::poller_del(SP_Channel request) {
    int fd = request->getFd();
#if defined(__linux__) || defined(__linux)
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getLastEvents();
    if (epoll_ctl(_pollerFD, EPOLL_CTL_DEL, fd, &event) < 0) {
        perror("epoll_del error");
    }
#endif

    _fdToChannel[fd].reset();
    _fdToHttp[fd].reset();
}

// 返回活跃事件数
std::vector<Poller::SP_Channel> Poller::poll() {
    while (true) {
#if defined(__linux__) || defined(__linux)
        int event_count = epoll_wait(_pollerFD, &*_events.begin(), _events.size(), POLLER_WAIT_TIME);
        if (event_count < 0) std::perror("epoll wait error");
        std::vector<Poller::SP_Channel> req_data = getEventRequest(event_count);
        if (req_data.size() > 0) return req_data;
#endif
    }
}

void Poller::add_timer(SP_Channel request_data, int timeout) {
    std::shared_ptr<HttpData> t = request_data->getHolder();
    if (t) {
        _timerManager.addTimer(t, timeout);
    } else {
        LOG_ERROR("timer add fail, fd : %d\n", request_data->getFd());
    }
}

void Poller::handleExpired() {
    _timerManager.handleExpiredevent();
}

std::vector<Poller::SP_Channel> Poller::getEventRequest(int events_num) {
    std::vector<Poller::SP_Channel> req_data;
    for(int i = 0 ; i < events_num ; i ++) {
        // 获取有事件产生的描述符
        int fd = _events[i].data.fd;
        SP_Channel cur_req = _fdToChannel[fd];

        if(cur_req) {
            cur_req -> setRevents(_events[i].events);
            cur_req -> setEvents(0);
            
            req_data.push_back(cur_req);
        } else {
            LOG_ERROR("SP channel cur_req is invalid\n", 0);
        }
    }
    return req_data;
}