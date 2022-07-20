#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpData.h"
#include "../Log/Logger.h"
#include "../Reactor/Channel.h"
#include "../Reactor/EventLoop.h"
#include "../Util/sockUtil.h"
#include <sys/epoll.h>
#include <cassert>

const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRED_TIME = 2000;             // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms

const char *HttpData::_srcDir = "";
std::atomic<int> HttpData::userCount = 0;
// static const char* _srcDir = "";
// static std::atomic<int> userCount = 0;

HttpData::HttpData(EventLoop *loop, int connfd) :
    _loop(loop),
    _channel(new Channel(loop, connfd)),
    _fd(connfd),
    _isClose(false),
    _connectionState(H_CONNECTED),
    _error(false),
    _keepAlive(false),
    _finished(false) {
    _channel->setReadHandler([this] { handleRead(); });
    _channel->setWriteHandle([this] { handleWrite(); });
    _channel->setConnHandler([this] { handleConn(); });
}

void HttpData::seperateTimer() {
    if (_timer.lock()) {
        std::shared_ptr<TimerNode> t(_timer.lock());
        t->clearReq();
        _timer.reset();
    }
}

void HttpData::handleRead() {
    __uint32_t &events = _channel->getEvents();
    do {
        bool zero = false;
        int read_num = readn(_fd, _inBuffer, zero);
        LOG_TRACE("Request inBuffer : %s\n", _inBuffer.c_str());
        if (_connectionState == H_DISCONNECTING) {
            _inBuffer.clear();
            break;
        }
        if (read_num < 0) {
            perror("read_num < 0");
            _error = true;
            handleError(_fd, 400, "Bad Request : can't read");
            break;
        } else if (zero) {
            // 有请求出现但是读不到数据，可能是Request
            // Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            // error_ = true;
            _connectionState = H_DISCONNECTING;
            if (read_num == 0) break;
        }

        bool parseSuccess = _request.parse(_inBuffer);
        _inBuffer.clear();
        if (!parseSuccess) {
            _error = true;
            handleError(_fd, 400, "Bad Request : parse request error");
            break;
        }
        
        // _keepAlive = _request.isKeepAlive();
        _response.Init(_srcDir, _request.path(), _keepAlive, 200);
        _response.MakeResponse(_outBuffer);
        if(_response.resErr()) {
            _error = true;
            handleError(_fd, 404, "File Not Found");
            break;
        }
        _finished = true;

    } while (false); // ET read 在 readn 中实现

    if (!_error) {
        if (_outBuffer.size() > 0) {
            handleWrite();
            _connectionState = H_DISCONNECTING;
        }
        // state of _error may change in handleWrite()
        // if (!_error && _request.GetParseState() == _request.FINISH) {
        if (!_error && _finished) {
            this->reset();
            if (_inBuffer.size() > 0) {
                if (_connectionState != H_DISCONNECTING) handleRead();
            }
        } else if (!_error && _connectionState != H_DISCONNECTED) {
            events |= EPOLLIN;
        }
    }
}

void HttpData::reset() {
    _request.Init();
    _finished = false;
    seperateTimer();
}

void HttpData::handleWrite() {
    if (!_error && _connectionState != H_DISCONNECTED) {
        __uint32_t &events = _channel->getEvents();
        if (writen(_fd, _outBuffer) < 0) {
            perror("writen error");
            events = 0;
            _error = true;
        }
        if (_outBuffer.size() > 0) events |= EPOLLOUT;
    }
}

void HttpData::handleConn() {
    seperateTimer();
    __uint32_t &events = _channel->getEvents();
    if (!_error && _connectionState == H_CONNECTED) {
        if (events != 0) {
            int timeout = DEFAULT_EXPIRED_TIME;
            if (_keepAlive) timeout = DEFAULT_KEEP_ALIVE_TIME;
            if ((events & EPOLLIN) && (events & EPOLLOUT)) {
                events = __uint32_t(0);
                events |= EPOLLOUT;
            }
            events |= EPOLLET;
            _loop->updatePoller(_channel, timeout);
        } else if (_keepAlive) {
            events |= (EPOLLIN | EPOLLET);
            int timeout = DEFAULT_KEEP_ALIVE_TIME;
            _loop->updatePoller(_channel, timeout);
        } else {
            events |= (EPOLLIN | EPOLLET);
            int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
            _loop->updatePoller(_channel, timeout);
        }
    } else if (!_error && _connectionState == H_DISCONNECTING && (events & EPOLLOUT)) {
        events = (EPOLLOUT | EPOLLET);
    } else {
        _loop->runInLoop([capture = shared_from_this()] { capture->handleClose(); });
    }
}

void HttpData::handleError(int fd, int err_num, std::string short_msg) {
    LOG_ERROR("HttpData handleError, fd : %d, ereCode : %d, msg : %s\n", fd, err_num, short_msg.c_str());
    // handleWrite();
    std::string body_buff, header_buff;
    char send_buff[4096];
    body_buff += "<html><title>ERROR</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_num) + short_msg;
    body_buff += "<hr><em> EasyServer </em>\n</body></html>";

    header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-Type: text/html\r\n";
    header_buff += "Connection: Close\r\n";
    header_buff += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
    header_buff += "Server: EasyServer\r\n";
    ;
    header_buff += "\r\n";
    // 错误处理不考虑writen不完的情况
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

void HttpData::handleClose() {
    HttpData::userCount--;
    LOG_INFO("Connection %d close, userCount : [%d]\n", _fd, static_cast<int>(HttpData::userCount));
    _connectionState = H_DISCONNECTED;
    std::shared_ptr<HttpData> guard(shared_from_this());
    _loop->removeFromPoller(_channel);
}

void HttpData::newEvent() {
    _channel->setEvents(DEFAULT_EVENT);
    _loop->addToPoller(_channel, DEFAULT_EXPIRED_TIME);
}
