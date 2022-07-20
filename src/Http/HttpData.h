#ifndef __SRC_HTTP_HTTPDATA_H_
#define __SRC_HTTP_HTTPDATA_H_

#include <memory>
#include <mutex>
#include <string>
#include <unistd.h>
#include <unordered_map>
// #include "../Http/HttpRequest.h"
// #include "../Http/HttpResponse.h"
#include <unordered_set>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

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
    HttpData(EventLoop *loop, int connfd);
    ~HttpData() {
        UnmapFile();
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

    // HttpRequest _request;
    //-------------------------------------------
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    PARSE_STATE _parseState;
    std::string _method, _path, _version, _body;
    std::unordered_map<std::string, std::string> _header;
    std::unordered_map<std::string, std::string> _post;
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);
    bool ParseRequestLine(const std::string &line);
    void ParseHeader(const std::string &line);
    void ParseBody(const std::string &line);
    void ParsePath();
    void ParsePost();
    void ParseFromUrlencoded();
    
    void initRequest();
    bool parse(std::string &buff);
    //-------------------------------------------

    //HttpResponse _response;
    int _code;
    char *_mmFile;
    struct stat _mmFileStat;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;

    void AddStateLine(std::string &buff);
    void AddHeader(std::string &buff);
    void AddContent(std::string &buff);

    void ErrorHtml();
    std::string GetFileType();

    void MakeResponse(std::string& buff);
    void handleErrorResponse(std::string& buff);
    void UnmapFile();
    char *File();
    size_t FileLen() const;
    void ErrorContent(std::string& buff, std::string messages);

    //-------------------------------------------
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
