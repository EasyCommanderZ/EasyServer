#ifndef __SRC_LOG_COUNTDOWNLATCH_H_
#define __SRC_LOG_COUNTDOWNLATCH_H_

#include "../Util/noncopyable.h"
#include <condition_variable>
#include <mutex>

// CountDownLatch
// 作用 ：  确保 Thread 中传进去的 func 真的启动了之后外层的 start 才返回
class CountDownLatch : noncopyable {
private:
    mutable std::mutex _mtx;
    std::condition_variable _cv;
    int _count;

public:
    explicit CountDownLatch(int _count);

    void wait();

    void countdown();

    int getCount();
};

#endif /* __SRC_LOG_COUNTDOWNLATCH_H_ */
