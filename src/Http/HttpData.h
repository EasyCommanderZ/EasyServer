#ifndef __SRC_HTTP_HTTPDATA_H_
#define __SRC_HTTP_HTTPDATA_H_
#include <atomic>
#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "../Reactor/Timer.h"

class EventLoop;
class TimerNode;
class Channel;

enum ProcessState {
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_ANALYSIS,
    STATE_FINISH
};

enum URIState {
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS,
};

enum HeaderState {
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum AnalysisState { ANALYSIS_SUCCESS = 1,
                     ANALYSIS_ERROR };

enum ParseState {
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

enum ConnectionState { H_CONNECTED = 0,
                       H_DISCONNECTING,
                       H_DISCONNECTED };

enum HttpMethod { METHOD_POST = 1,
                  METHOD_GET,
                  METHOD_HEAD };

enum HttpVersion { HTTP_10 = 1,
                   HTTP_11 };

class HttpData : public std::enable_shared_from_this<HttpData> {
public:
    HttpData(EventLoop *loop, int connfd);
    ~HttpData() {
        close(fd_);
    }
    void reset();
    void seperateTimer();
    void linkTimer(std::shared_ptr<TimerNode> mtimer) {
        // shared_ptr重载了bool, 但weak_ptr没有
        timer_ = mtimer;
    }
    std::shared_ptr<Channel> getChannel() {
        return channel_;
    }
    EventLoop *getLoop() {
        return loop_;
    }
    void handleClose();
    void newEvent();
    static std::atomic_int userCount;

private:
    EventLoop *loop_;
    std::shared_ptr<Channel> channel_;
    int fd_;
    std::string inBuffer_;
    std::string outBuffer_;
    bool error_;
    ConnectionState connectionState_;

    HttpMethod method_;
    HttpVersion HTTPVersion_;
    std::string fileName_;
    std::string path_;
    int nowReadPos_;
    ProcessState state_;
    ParseState hState_;
    bool keepAlive_;
    std::map<std::string, std::string> headers_;
    std::weak_ptr<TimerNode> timer_;

    void handleRead();
    void handleWrite();
    void handleConn();
    void handleError(int fd, int err_num, std::string short_msg);
    URIState parseURI();
    HeaderState parseHeaders();
    AnalysisState analysisRequest();

    static std::unordered_map<std::string, std::string> SUFFIX_TYPE;
};

#endif /* __SRC_HTTP_HTTPDATA_H_ */
