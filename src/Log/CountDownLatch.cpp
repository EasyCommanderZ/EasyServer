#include "Log/CountDownLatch.h"
#include <mutex>

CountDownLatch::CountDownLatch(int count) : _count(count) {}

void CountDownLatch::wait() {
    std::unique_lock lck(_mtx);
    while(_count > 0) {
        _cv.wait(lck);
    }
}

void CountDownLatch::countdown() {
    std::unique_lock lck(_mtx);
    if( -- _count == 0) {
        _cv.notify_all();
    }
}

int CountDownLatch::getCount() {
    std::unique_lock lck(_mtx);
    return _count;
}