#ifndef __SRC_POLLER_CHANNEL_H_
#define __SRC_POLLER_CHANNEL_H_

#include <cstdint>
#include <functional>
#include <cstddef>

// support linux && macOS
// marco template:
// #if defined(__linux__) || defined(__linux) //__linux__
// #elif defined(__APPLE__) || defined(__APPLE)
// #endif

// epoll中的 epoll_data是一个联合类型，可以存储一个指针。这样可以将一个fd封装成一个Channel类，一个Channel类负责一个文件描述符，对不同的服务、不同的事件类型，都可以在类中进行不同的处理，而不仅仅是拿到一个int类型的fd

class Channel {
private:
    using CallBack = std::function<void()>;
    int _fd;
    u_int32_t _events;
    std::uint32_t _revents;
    std::uint32_t _lastEvents;

};
#endif /* __SRC_POLLER_CHANNEL_H_ */
