#include "Server.h"
#include "../Http/HttpData.h"
#include "../Log/Logger.h"
#include "../Reactor/Channel.h"
#include "../Reactor/EventLoop.h"
#include "../Reactor/EventLoopThreadPool.h"
#include "../Util/sockUtil.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(EventLoop *loop, int threadNum, int port) :
    _mainLoop(loop), _threadNum(threadNum), _eventLoopPool(new EventLoopThreadPool(_mainLoop, _threadNum)), _started(false), _acceptChannel(new Channel(_mainLoop)), _port(port), _listenFd(socket_bind_listen(_port)) {
    _acceptChannel->setFd(_listenFd);
    handle_for_sigpipe();
    if (setSocketNonBlocking(_listenFd) < 0) {
        perror("set socket non-block failed \n");
        abort();
    }
}

void Server::start() {
    _eventLoopPool->start();
    _acceptChannel->setEvents(EPOLLIN | EPOLLET);
    _acceptChannel->setReadHandler([this] { handleNewConn(); });
    _acceptChannel->setConnHandler([this] { handleConn(); });
    _mainLoop->addToPoller(_acceptChannel, 0);
    _started = true;
    LOG_INFO("EasyServer STARTED\n", 0);
}

void Server::handleNewConn() {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while ((accept_fd = accept(_listenFd, (struct sockaddr *)&client_addr, &client_addr_len)) > 0) {
        EventLoop *loop = _eventLoopPool->getNextLoop();

        // 限制服务器最大并发连接数
        if (accept_fd >= MAXFDS) {
            close(accept_fd);
            continue;
        }
        // 设为非阻塞模式
        if (setSocketNonBlocking(accept_fd) < 0) {
            LOG_ERROR("Set NON-BLOCK failed!\n", 0);
            return;
        }

        setSocketNodelay(accept_fd);
        // setSocketNoLinger(accept_fd);

        std::shared_ptr<Channel> req_channel(new Channel(loop, accept_fd));
        std::shared_ptr<HttpData> req_info(new HttpData(req_channel, accept_fd));
        loop -> queueInLoop([req_info] { req_info->newEvent(); });
        HttpData::userCount ++;
        LOG_INFO("New Connection from %s : %d, userCount : [%d]", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), static_cast<int>(HttpData::userCount));
    }
    _acceptChannel -> setEvents(EPOLLIN | EPOLLET);
}