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
// #include "HttpRequest.h"
// #include "HttpResponse.h"
#include "HttpData.h"
#include "../Log/Logger.h"
#include "../Reactor/Channel.h"
#include "../Reactor/EventLoop.h"
#include "../Util/sockUtil.h"
#include <sys/epoll.h>
#include <cassert>
#include <regex>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

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
    initRequest();
    if (_mmFile) {
        UnmapFile();
    }
    _code = -1;
    _mmFile = nullptr;
    _mmFileStat = {0};
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

        bool parseSuccess = parse(_inBuffer);
        _inBuffer.clear();
        if (!parseSuccess) {
            _error = true;
            handleError(_fd, 400, "Bad Request : parse request error");
            break;
        }
        
        // _keepAlive = _request.isKeepAlive();
        // _response.Init(_srcDir, _path, _keepAlive, 200);
        MakeResponse(_outBuffer);
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
    initRequest();
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
    body_buff += std::to_string(err_num) + " " +short_msg;
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

// reconstruct HttpData into one class
// --------------------------------------
const std::unordered_set<std::string> HttpData::DEFAULT_HTML{
    "/index",
    "/welcome",
    "/video",
    "/picture",
    "/register",
    "/login",
};

const std::unordered_map<std::string, int> HttpData::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

void HttpData::initRequest() {
    _method = _path = _version = _body = "";
    _parseState = REQUEST_LINE;
    _header.clear();
    _post.clear();
}

bool HttpData::ParseRequestLine(const std::string &line) {
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern)) {
        _method = subMatch[1];
        _path = subMatch[2];
        _version = subMatch[3];
        _parseState = HEADERS;
        return true;
    }
    LOG_ERROR("RequstLine Error\n", 0);
    return false;
}

void HttpData::ParsePath() {
    if (_path == "/") {
        _path = "/index.html";
    } else {
        for (auto &item : DEFAULT_HTML) {
            if (item == _path) {
                _path += ".html";
                break;
            }
        }
    }
}

void HttpData::ParseHeader(const std::string &line) {
    std::regex patten("([\\w-]+): (.*)");
    std::smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        _header[subMatch[1]] = subMatch[2];
    } else {
        _parseState = BODY;
    }
}

void HttpData::ParseFromUrlencoded() {
    if (_body.size() == 0) return;
    std::string key, value;
    int num = 0, n = _body.size();
    int i = 0, j = 0;

    for (; i < n; i++) {
        char ch = _body[i];
        switch (ch) {
        case '=':
            key = _body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            _body[i] = ' ';
            break;
        case '%':
            num = ConverHex(_body[i + 1]) * 16 + ConverHex(_body[i + 2]);
            _body[i + 2] = num % 10 + '0';
            _body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = _body.substr(j, i - j);
            j = i + 1;
            _post[key] = value;
            LOG_TRACE("%s = %s \n", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if (_post.count(key) == 0 && j < i) {
        value = _body.substr(j, i - j);
        _post[key] = value;
    }
}

// Add more post pages;
void HttpData::ParsePost() {
    if (_method == "POST" && _header["Content-Type"] == "applicaton/x-www-form-urlencoded'") {
        LOG_TRACE("Post method, path : %s\n", _path.c_str());
        ParseFromUrlencoded();
        // if (DEFAULT_HTML_TAG.count(_path)) {
        //     int tag = DEFAULT_HTML_TAG.find(_path) -> second;
        //     switch (tag) {

        //     }
        // }
    }
}

void HttpData::ParseBody(const std::string &line) {
    _body = line;
    ParsePost();
    _parseState = FINISH;
    LOG_TRACE("Body:%s, len:%d\n", line.c_str(), line.size());
}

bool HttpData::parse(std::string &buff) {
    const char CRLF[] = "\r\n";
    if (buff.empty()) return false;
    auto cur = buff.begin();
    while (cur != buff.end() && _parseState != FINISH) {
        auto lineEnd = std::search(cur, buff.end(), CRLF, CRLF + 2);
        std::string line(cur, lineEnd);
        cur = lineEnd + 2;
        switch (_parseState) {
        case REQUEST_LINE:
            if (!ParseRequestLine(line)) {
                return false;
            }
            ParsePath();
            break;
        case HEADERS:
            ParseHeader(line);
            if (buff.end() - cur <= 3) {
                _parseState = FINISH;
            }
            break;
        case BODY:
            ParseBody(line);
            break;
        default: break;
        }
        if (lineEnd == buff.end()) break;
        // buff.RetrieveUntil(lineEnd + 2);
        // buff.clear();
    }
    if (_header.count("Connection") == 1) {
        _keepAlive = ( _header.find("Connection")->second == "keep-alive" && _version == "1.1");
    }
    LOG_TRACE("[%s], [%s], [%s] \n", _method.c_str(), _path.c_str(), _version.c_str());
    return true;
}

int HttpData::ConverHex(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}

// --------------------------------------

// reconstruct httpresponse
// --------------------------------------
const std::unordered_map<std::string, std::string> HttpData::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const std::unordered_map<int, std::string> HttpData::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpData::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

char *HttpData::File() {
    return _mmFile;
}

size_t HttpData::FileLen() const {
    return _mmFileStat.st_size;
}

void HttpData::ErrorHtml() {
    if (CODE_PATH.count(_code) == 1) {
        _path = CODE_PATH.find(_code)->second;
        stat((_srcDir + _path).data(), &_mmFileStat);
    }
}

void HttpData::AddStateLine(std::string &buff) {
    std::string status;
    if (CODE_STATUS.count(_code) == 1) {
        status = CODE_STATUS.find(_code)->second;
    } else {
        _code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff += ("HTTP/1.1 " + std::to_string(_code) + " " + status + "\r\n");
}

void HttpData::AddHeader(std::string &buff) {
    buff += "Connection: ";
    if (_keepAlive) {
        buff += "keep-alive\r\n";
        buff += "keep-alive: timeout=" + std::to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
    } else {
        buff += "close\r\n";
    }
    buff += "Content-type: " + GetFileType() + "\r\n";
    buff += "Server: EasyServer\r\n";
}

void HttpData::AddContent(std::string &buff) {
    int srcFd = open((_srcDir + _path).data(), O_RDONLY);
    if (srcFd < 0) {
        _error = true;
        _code = 404;
        // ErrorContent(buff, "File Not Found");
        handleError(_fd, 404, "File Not Found");
        return;
    }

    /* 将文件映射到内存提高文件的访问速度
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    int *mmRet = (int *)mmap(0, _mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    close(srcFd);
    LOG_TRACE("file path %s \n", (_srcDir + _path).data());
    if (*mmRet == -1) {
        _error = true;
        // ErrorContent(buff, "File Not Found!");
        handleError(_fd, 404, "File Not Found");
        return;
    }
    _mmFile = (char *)mmRet;
    buff += "Content-length: " + std::to_string(_mmFileStat.st_size) + "\r\n\r\n";
    buff += std::string(_mmFile, _mmFile + _mmFileStat.st_size);
}

void HttpData::UnmapFile() {
    if (_mmFile) {
        munmap(_mmFile, _mmFileStat.st_size);
        _mmFile = nullptr;
    }
}

std::string HttpData::GetFileType() {
    /* 判断文件类型 */
    std::string::size_type idx = _path.find_last_of('.');
    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = _path.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpData::ErrorContent(std::string &buff, std::string message) {
    std::string body;
    std::string status;

    body += "<html><title>QAQ EASY NOT EASY</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(_code) == 1) {
        status = CODE_STATUS.find(_code)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>EasyServer</em>\n</body></html>";
    buff += "Content-length: " + std::to_string(body.size()) + "\r\n\r\n";
    buff += body;
}

void HttpData::MakeResponse(std::string &buff) {

    /* 判断请求的资源文件 */
    if (stat((_srcDir + _path).data(), &_mmFileStat) < 0 || S_ISDIR(_mmFileStat.st_mode)) {
        _code = 404;
    } else if (!(_mmFileStat.st_mode & S_IROTH)) {
        _code = 403;
    } else if (_code == -1) {
        _code = 200;
    }

    ErrorHtml();
    AddStateLine(buff);
    AddHeader(buff);
    AddContent(buff);
}

// --------------------------------------