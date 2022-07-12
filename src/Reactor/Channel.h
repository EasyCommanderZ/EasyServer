#ifndef __SRC_REACTOR_CHANNEL_H_
#define __SRC_REACTOR_CHANNEL_H_

#include "../Http/HttpData.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <cstddef>
#include <memory>
#include <sys/epoll.h>

// support linux && macOS
// marco template:
// #if defined(__linux__) || defined(__linux) //__linux__
// #elif defined(__APPLE__) || defined(__APPLE)
// #endif

// epoll中的 epoll_data是一个联合类型，可以存储一个指针。这样可以将一个fd封装成一个Channel类，一个Channel类负责一个文件描述符，对不同的服务、不同的事件类型，都可以在类中进行不同的处理，而不仅仅是拿到一个int类型的fd

// Channel 是 Reactor 结构中的 “事件”，它始终属于一个 EventLoop，负责一个文件描述符的IO事件，在 Channel 类中保存这个 IO 事件的类型以及对应的回调函数。当 IO 事件发生时候，最终会调用 Channel 类中的回调函数。
// channel 和 loop中的 eventfd listenfd HttpData 都有关联

class EventLoop;
class Channel {
private:
    using CallBack = std::function<void()>;

    EventLoop *_loop;
    int _fd;
    std::uint32_t _events; // 表示希望监听这个描述符的哪些事件
    std::uint32_t _revents; // epoll 返回该 channel 时文件描述符正在发生的事件
    std::uint32_t _lastEvents; // 
    std::weak_ptr<HttpData> _holder; // 便于找到上层持有该 Channel 的连接
    
    CallBack _readHandler;
    CallBack _writeHandler;
    CallBack _errorHandler;
    CallBack _connHandler;

public:
    using SP_Channel = std::shared_ptr<Channel>;

    Channel(EventLoop *loop);
    Channel(EventLoop *loop, int fd);
    ~Channel();
    int getFd();
    void setFd(int fd);

    EventLoop* getLoop() {
        return _loop;
    }

    void setHolder(std::shared_ptr<HttpData> holder) {
        _holder = holder;
    }

    std::shared_ptr<HttpData> getHolder() {
        std::shared_ptr<HttpData> ret(_holder.lock());
        return ret;
    }

    void setReadHandler(CallBack &&readHandler) {
        _readHandler = std::move(readHandler);
    }

    void setWriteHandle(CallBack &&writeHandler) {
        _writeHandler = std::move(writeHandler);
    }

    void setErrorHandler(CallBack &&errorHandler) {
        _errorHandler = std::move(errorHandler);
    }

    void setConnHandler(CallBack &&connHandler) {
        _connHandler = std::move(connHandler);
    }

    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();

    void handleEvents() {
        _events = 0;
        if((_revents & EPOLLHUP) && !(_revents & EPOLLIN)) {
            _events = 0;
            return ;
        }
        if(_revents & EPOLLERR) {
            if(_errorHandler) _errorHandler();
            _events = 0;
            return;
        }
        if(_revents & (EPOLLIN | EPOLLPRI | EPOLLHUP)) {
            handleRead();
        }
        if(_revents & EPOLLOUT) {
            handleWrite();
        }
        handleConn();
    }

    void setRevents(std::uint32_t ev) {
        _revents = ev;
    }

    void setEvents(std::uint32_t ev) {
        _events = ev;
    }

    std::uint32_t& getEvents() {
        return _events;
    }

    bool equalAndUpdateEvents() {
        bool ret = (_lastEvents == _events);
        _lastEvents = _events;
        return ret;
    }

    std::uint32_t getLastEvents() {
        return _lastEvents;
    }

};
#endif /* __SRC_REACTOR_CHANNEL_H_ */
