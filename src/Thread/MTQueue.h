#ifndef __SRC_THREADPOOL_MTQUEUE_H_
#define __SRC_THREADPOOL_MTQUEUE_H_

// 一个线程安全的队列，提供基本的入队和出队操作
// 源代码参考B站小彭老师（@双笙子佯谬）的视频课
// 此处实现的比较简单，仅提供了队头出队和队尾入队的功能

#include <condition_variable>
#include <mutex>
#include <queue>
template <class T>
class MTQueue {
    std::condition_variable m_cv;
    std::mutex m_mtx;
    // std::vector<T> m_arr;
    std::queue<T> m_arr;

public:
    T pop() {
        std::unique_lock lck(m_mtx);
        m_cv.wait(lck, [this] { return !m_arr.empty(); });
        T ret = std::move(m_arr.front());
        m_arr.pop();
        return ret;
    }

    void push_back(T val) {
        std::unique_lock lck(m_mtx);
        m_arr.emplace(std::move(val));
        m_cv.notify_one();
    }

    size_t size() const {
        std::unique_lock lck(m_mtx);
        return m_arr.size();
    }
};

#endif /* __SRC_THREADPOOL_MTQUEUE_H_ */
