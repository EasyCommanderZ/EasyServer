#ifndef __SRC_POLLER_TIMER_H_
#define __SRC_POLLER_TIMER_H_

// #include "../Http/HttpData.h"
#pragma once
#include "Http/HttpData.h"
#include <cstddef>
#include <deque>
#include <memory>
#include <queue>

class HttpData;
class TimerNode {
private:
    bool _deleted;
    size_t _expiredTime;
    std::shared_ptr<HttpData> SPHttpData;

public:
    TimerNode(std::shared_ptr<HttpData> requestData, int timeout);
    ~TimerNode();
    TimerNode(TimerNode &tn);
    void update(int timeout);
    bool isValid();
    void clearReq();
    void setDeleted() {
        _deleted = true;
    }
    bool isDeleted() const {
        return _deleted;
    }
    size_t getExpTime() const {
        return _expiredTime;
    }
};

struct TimerCmp {
    bool operator()(std::shared_ptr<TimerNode> &a, std::shared_ptr<TimerNode> &b) const {
        return a->getExpTime() > b->getExpTime();
    }
};

class TimerManager {
private:
    using SPTimerNode = std::shared_ptr<TimerNode>;
    std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> _timerQueue;

public:
    TimerManager();
    ~TimerManager();
    void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout);
    void handleExpiredevent();
};

#endif /* __SRC_POLLER_TIMER_H_ */
