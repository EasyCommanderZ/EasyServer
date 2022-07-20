#ifndef __SRC_HTTP_HTTPDATA_H_
#define __SRC_HTTP_HTTPDATA_H_

#include <memory>
#include <mutex>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include "../Http/HttpRequest.h"
#include "../Http/HttpResponse.h"

class EventLoop;
class Channel;
class TimerNode;

class HttpData : public std::enable_shared_from_this<HttpData> {
public:
    enum ConnectionState { H_CONNECTED = 0,
                           H_DISCONNECTING,
                           H_DISCONNECTED };
    using SP_Channel = std::shared_ptr<Channel>;
    
    static const char *_srcDir;
    static std::atomic<int> userCount;

    // HttpData(SP_Channel channel, int connfd);
    HttpData(EventLoop* loop, int connfd);
    ~HttpData() {
        close(_fd);
    };
    void reset();

    void seperateTimer();
    void linkTimer(std::shared_ptr<TimerNode> mtimer) {
        _timer = mtimer;
    }
    SP_Channel getChannel() {
        return _channel;
    };
    void handleClose();
    void newEvent();

private:
    EventLoop *_loop;
    SP_Channel _channel;
    int _fd;

    HttpRequest _request;
    HttpResponse _response;

    // Buffer _inBuffer;
    // Buffer _outBuffer;
    std::string _inBuffer;
    std::string _outBuffer;
    bool _isClose;
    bool _error;
    ConnectionState _connectionState;
    bool _keepAlive;
    bool _finished;

    std::weak_ptr<TimerNode> _timer;

    void handleRead();
    void handleWrite();
    void handleConn();
    void handleError(int fd, int err_num, std::string short_msg);
};

#endif /* __SRC_HTTP_HTTPDATA_H_ */
