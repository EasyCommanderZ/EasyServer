
#include "Reactor/Channel.h"
#include "Reactor/EventLoop.h"

Channel::Channel(EventLoop *loop) :
    _loop(loop), _events(0), _lastEvents(0), _fd(0){};

Channel::Channel(EventLoop *loop, int fd) :
    _loop(loop), _fd(fd), _events(0), _lastEvents(0){};

Channel::~Channel() = default;

int Channel::getFd() {
    return _fd;
}

void Channel::setFd(int fd) {
    _fd = fd;
}

void Channel::handleRead() {
    if(_readHandler) {
        _readHandler();
    }
}

void Channel::handleConn() {
    if(_connHandler) {
        _connHandler();
    }
}

void Channel::handleWrite() {
    if(_writeHandler) {
        _writeHandler();
    }
}