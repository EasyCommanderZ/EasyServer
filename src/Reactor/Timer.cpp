#include "Timer.h"
#include "../Util/Timestamp.h"
#include <cstddef>
#include <ctime>

TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout) :
    _deleted(false), SPHttpData(requestData) {
    _expiredTime = Timestamp::now().getMilliSeconds() + timeout;
}

TimerNode::~TimerNode() {
    if (SPHttpData) SPHttpData->handleClose();
}

TimerNode::TimerNode(TimerNode &tn) :
    SPHttpData(tn.SPHttpData), _expiredTime(0) {
}

void TimerNode::update(int timeout) {
    _expiredTime = Timestamp::now().getMilliSeconds() + timeout;
}

bool TimerNode::isValid() {
    size_t tmp = Timestamp::now().getMilliSeconds();
    if(tmp < _expiredTime) {
        return true;
    }
    this -> setDeleted();
    return false;
}

void TimerNode::clearReq() {
    SPHttpData->reset();
    this -> setDeleted();
}

TimerManager::TimerManager() = default;
TimerManager::~TimerManager() = default;

void TimerManager::addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout) {
    SPTimerNode newNode(new TimerNode(SPHttpData, timeout));
    _timerQueue.push(newNode);
    SPHttpData -> linkTimer(newNode);
}

// priority_queue 不支持随机访问，被设置为 deleted 的节点会延迟到它 超时 或者 他前面的节点都被删除时， 才会删除
void TimerManager::handleExpiredevent() {
    while(!_timerQueue.empty()) {
        SPTimerNode tmp = _timerQueue.top();
        if(tmp -> isDeleted() || !tmp -> isValid()) _timerQueue.pop();
        else break;
    }
}