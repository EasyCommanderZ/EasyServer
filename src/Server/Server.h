#ifndef __SRC_SERVER_SERVER_H_
#define __SRC_SERVER_SERVER_H_

#include "Reactor/EventLoop.h"
#include "Reactor/EventLoopThreadPool.h"
#include <memory>
class Server {
public:
    Server(EventLoop *loop, int threadNum, int port);
    ~Server() = default;
    EventLoop *getLoop() const;
    void start();
    void handleNewConn();
    void handleConn();

private:
    EventLoop* _mainLoop;
    int _threadNum;
    std::unique_ptr<EventLoopThreadPool> _eventLoopPool;
    bool _started;
    std::shared_ptr<Channel> _acceptChannel;
    int _port;
    int _listenFd;
    static const int MAXFDS = 100000;
};

#endif /* __SRC_SERVER_SERVER_H_ */
