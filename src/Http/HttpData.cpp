// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "HttpData.h"
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include "../Reactor/Channel.h"
#include "../Reactor/EventLoop.h"
#include "../Util/sockUtil.h"
#include <ctime>
#include <unordered_map>
#include "../Log/Logger.h"

using namespace std;

unordered_map<string, string> HttpData::SUFFIX_TYPE = {
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
    {"default", "text/html"},
};

atomic_int HttpData::userCount = 0;

const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRED_TIME = 2000;             // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms

HttpData::HttpData(EventLoop *loop, int connfd) :
    loop_(loop),
    channel_(new Channel(loop, connfd)),
    fd_(connfd),
    error_(false),
    connectionState_(H_CONNECTED),
    method_(METHOD_GET),
    HTTPVersion_(HTTP_11),
    nowReadPos_(0),
    state_(STATE_PARSE_URI),
    hState_(H_START),
    keepAlive_(false) {
    // loop_->queueInLoop(bind(&HttpData::setHandlers, this));
    channel_->setReadHandler([this] { handleRead(); });
    channel_->setWriteHandler([this] { handleWrite(); });
    channel_->setConnHandler([this] { handleConn(); });
}

void HttpData::reset() {
    // inBuffer_.clear();
    fileName_.clear();
    path_.clear();
    nowReadPos_ = 0;
    state_ = STATE_PARSE_URI;
    hState_ = H_START;
    headers_.clear();
    // keepAlive_ = false;
    seperateTimer();
}

void HttpData::seperateTimer() {
    if (timer_.lock()) {
        shared_ptr<TimerNode> my_timer(timer_.lock());
        my_timer->clearReq();
        timer_.reset();
    }
}

void HttpData::handleRead() {
    __uint32_t &events_ = channel_->getEvents();
    do {
        bool zero = false;
        int read_num = readn(fd_, inBuffer_, zero);
        LOG_TRACE("Reqeust: %s\n", inBuffer_.c_str());
        if (connectionState_ == H_DISCONNECTING) {
            inBuffer_.clear();
            break;
        }

        if (read_num < 0) {
            perror("1");
            error_ = true;
            handleError(fd_, 400, "Bad Request");
            break;
        }

        else if (zero) {
            // 有请求出现但是读不到数据，可能是Request
            // Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            // error_ = true;
            connectionState_ = H_DISCONNECTING;
            if (read_num == 0) {
                // error_ = true;
                break;
            }
            // cout << "readnum == 0" << endl;
        }

        if (state_ == STATE_PARSE_URI) {
            URIState flag = this->parseURI();
            if (flag == PARSE_URI_AGAIN)
                break;
            else if (flag == PARSE_URI_ERROR) {
                perror("2");
                inBuffer_.clear();
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            } else
                state_ = STATE_PARSE_HEADERS;
        }
        if (state_ == STATE_PARSE_HEADERS) {
            HeaderState flag = this->parseHeaders();
            if (flag == PARSE_HEADER_AGAIN)
                break;
            else if (flag == PARSE_HEADER_ERROR) {
                perror("3");
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            }
            if (method_ == METHOD_POST) {
                // POST方法准备
                state_ = STATE_RECV_BODY;
            } else {
                state_ = STATE_ANALYSIS;
            }
        }
        if (state_ == STATE_RECV_BODY) {
            int content_length = -1;
            if (headers_.find("Content-length") != headers_.end()) {
                content_length = stoi(headers_["Content-length"]);
            } else {
                // cout << "(state_ == STATE_RECV_BODY)" << endl;
                error_ = true;
                handleError(fd_, 400, "Bad Request: Lack of argument (Content-length)");
                break;
            }
            if (static_cast<int>(inBuffer_.size()) < content_length) break;
            state_ = STATE_ANALYSIS;
        }
        if (state_ == STATE_ANALYSIS) {
            AnalysisState flag = this->analysisRequest();
            if (flag == ANALYSIS_SUCCESS) {
                state_ = STATE_FINISH;
                break;
            } else {
                // cout << "state_ == STATE_ANALYSIS" << endl;
                error_ = true;
                break;
            }
        }
    } while (false);
    // cout << "state_=" << state_ << endl;
    if (!error_) {
        if (outBuffer_.size() > 0) {
            handleWrite();
            // events_ |= EPOLLOUT;
        }
        // error_ may change
        if (!error_ && state_ == STATE_FINISH) {
            this->reset();
            if (inBuffer_.size() > 0) {
                if (connectionState_ != H_DISCONNECTING) handleRead();
            }

            // if ((keepAlive_ || inBuffer_.size() > 0) && connectionState_ ==
            // H_CONNECTED)
            // {
            //     this->reset();
            //     events_ |= EPOLLIN;
            // }
        } else if (!error_ && connectionState_ != H_DISCONNECTED)
            events_ |= EPOLLIN;
    }
}

void HttpData::handleWrite() {
    if (!error_ && connectionState_ != H_DISCONNECTED) {
        __uint32_t &events_ = channel_->getEvents();
        if (writen(fd_, outBuffer_) < 0) {
            perror("writen");
            events_ = 0;
            error_ = true;
        }
        if (outBuffer_.size() > 0) events_ |= EPOLLOUT;
    }
}

void HttpData::handleConn() {
    seperateTimer();
    __uint32_t &events_ = channel_->getEvents();
    if (!error_ && connectionState_ == H_CONNECTED) {
        if (events_ != 0) {
            int timeout = DEFAULT_EXPIRED_TIME;
            if (keepAlive_) timeout = DEFAULT_KEEP_ALIVE_TIME;
            if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {
                events_ = __uint32_t(0);
                events_ |= EPOLLOUT;
            }
            events_ |= EPOLLET;
            loop_->updatePoller(channel_, timeout);

        } else if (keepAlive_) {
            events_ |= (EPOLLIN | EPOLLET);
            int timeout = DEFAULT_KEEP_ALIVE_TIME;
            loop_->updatePoller(channel_, timeout);
        } else {
            events_ |= (EPOLLIN | EPOLLET);
            int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
            loop_->updatePoller(channel_, timeout);
        }
    } else if (!error_ && connectionState_ == H_DISCONNECTING && (events_ & EPOLLOUT)) {
        events_ = (EPOLLOUT | EPOLLET);
    } else {
        // cout << "close with errors" << endl;
        loop_->runInLoop([capture0 = shared_from_this()] { capture0->handleClose(); });
    }
}

URIState HttpData::parseURI() {
    string &str = inBuffer_;
    string cop = str;
    // 读到完整的请求行再开始解析请求
    size_t pos = str.find('\r', nowReadPos_);
    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    string request_line = str.substr(0, pos);
    if (str.size() > pos + 1)
        str = str.substr(pos + 1);
    else
        str.clear();
    // Method
    int posGet = request_line.find("GET");
    int posPost = request_line.find("POST");
    int posHead = request_line.find("HEAD");

    if (posGet >= 0) {
        pos = posGet;
        method_ = METHOD_GET;
    } else if (posPost >= 0) {
        pos = posPost;
        method_ = METHOD_POST;
    } else if (posHead >= 0) {
        pos = posHead;
        method_ = METHOD_HEAD;
    } else {
        return PARSE_URI_ERROR;
    }

    // filename
    pos = request_line.find("/", pos);
    if (pos < 0) {
        fileName_ = "index.html";
        HTTPVersion_ = HTTP_11;
        return PARSE_URI_SUCCESS;
    } else {
        size_t _pos = request_line.find(' ', pos);
        if (_pos < 0)
            return PARSE_URI_ERROR;
        else {
            if (_pos - pos > 1) {
                fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
                size_t __pos = fileName_.find('?');
                if (__pos >= 0) {
                    fileName_ = fileName_.substr(0, __pos);
                }
            } else
                fileName_ = "index.html";
        }
        pos = _pos;
    }
    // cout << "fileName_: " << fileName_ << endl;
    // HTTP 版本号
    pos = request_line.find("/", pos);
    if (pos < 0)
        return PARSE_URI_ERROR;
    else {
        if (request_line.size() - pos <= 3)
            return PARSE_URI_ERROR;
        else {
            string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
                HTTPVersion_ = HTTP_10;
            else if (ver == "1.1")
                HTTPVersion_ = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }
    return PARSE_URI_SUCCESS;
}

HeaderState HttpData::parseHeaders() {
    string &str = inBuffer_;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    size_t i = 0;
    for (; i < str.size() && notFinish; ++i) {
        switch (hState_) {
        case H_START: {
            if (str[i] == '\n' || str[i] == '\r') break;
            hState_ = H_KEY;
            key_start = i;
            now_read_line_begin = i;
            break;
        }
        case H_KEY: {
            if (str[i] == ':') {
                key_end = i;
                if (key_end - key_start <= 0) return PARSE_HEADER_ERROR;
                hState_ = H_COLON;
            } else if (str[i] == '\n' || str[i] == '\r')
                return PARSE_HEADER_ERROR;
            break;
        }
        case H_COLON: {
            if (str[i] == ' ') {
                hState_ = H_SPACES_AFTER_COLON;
            } else
                return PARSE_HEADER_ERROR;
            break;
        }
        case H_SPACES_AFTER_COLON: {
            hState_ = H_VALUE;
            value_start = i;
            break;
        }
        case H_VALUE: {
            if (str[i] == '\r') {
                hState_ = H_CR;
                value_end = i;
                if (value_end - value_start <= 0) return PARSE_HEADER_ERROR;
            } else if (i - value_start > 255)
                return PARSE_HEADER_ERROR;
            break;
        }
        case H_CR: {
            if (str[i] == '\n') {
                hState_ = H_LF;
                string key(str.begin() + key_start, str.begin() + key_end);
                string value(str.begin() + value_start, str.begin() + value_end);
                headers_[key] = value;
                now_read_line_begin = i;
            } else
                return PARSE_HEADER_ERROR;
            break;
        }
        case H_LF: {
            if (str[i] == '\r') {
                hState_ = H_END_CR;
            } else {
                key_start = i;
                hState_ = H_KEY;
            }
            break;
        }
        case H_END_CR: {
            if (str[i] == '\n') {
                hState_ = H_END_LF;
            } else
                return PARSE_HEADER_ERROR;
            break;
        }
        case H_END_LF: {
            notFinish = false;
            key_start = i;
            now_read_line_begin = i;
            break;
        }
        }
    }
    if (hState_ == H_END_LF) {
        str = str.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

AnalysisState HttpData::analysisRequest() {
    if (method_ == METHOD_POST) {
        // add post request analysis here

    } else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
        string header;
        header += "HTTP/1.1 200 OK\r\n";
        if (headers_.find("Connection") != headers_.end() && (headers_["Connection"] == "Keep-Alive" || headers_["Connection"] == "keep-alive")) {
            keepAlive_ = true;
            header += string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        }
        int dot_pos = fileName_.find('.');
        string filetype;
        if (dot_pos < 0)
            filetype = HttpData::SUFFIX_TYPE["default"];
        else
            filetype = HttpData::SUFFIX_TYPE[fileName_.substr(dot_pos)];

        // echo test
        if (fileName_ == "benchtest") {
            outBuffer_ =
                "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return ANALYSIS_SUCCESS;
        }

        struct stat sbuf;
        if (stat(fileName_.c_str(), &sbuf) < 0) {
            header.clear();
            handleError(fd_, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        header += "Content-Type: " + filetype + "\r\n";
        header += "Content-Length: " + to_string(sbuf.st_size) + "\r\n";
        header += "Server: EasyServer\r\n";
        // 头部结束
        header += "\r\n";
        outBuffer_ += header;

        if (method_ == METHOD_HEAD) return ANALYSIS_SUCCESS;

        int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
        if (src_fd < 0) {
            outBuffer_.clear();
            handleError(fd_, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
        close(src_fd);
        if (mmapRet == (void *)-1) {
            munmap(mmapRet, sbuf.st_size);
            outBuffer_.clear();
            handleError(fd_, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        char *src_addr = static_cast<char *>(mmapRet);
        outBuffer_ += string(src_addr, src_addr + sbuf.st_size);
        ;
        munmap(mmapRet, sbuf.st_size);
        return ANALYSIS_SUCCESS;
    }
    return ANALYSIS_ERROR;
}

void HttpData::handleError(int fd, int err_num, string short_msg) {
    short_msg = " " + short_msg;
    char send_buff[4096];
    string body_buff, header_buff;
    body_buff += "<html><title>Error Page</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += to_string(err_num) + short_msg;
    body_buff += "<hr><em> EasyServer </em>\n</body></html>";

    header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-Type: text/html\r\n";
    header_buff += "Connection: Close\r\n";
    header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
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
    LOG_TRACE("Connection %d close, userCount : [%d]\n", fd_, static_cast<int>(HttpData::userCount));
    connectionState_ = H_DISCONNECTED;
    shared_ptr<HttpData> guard(shared_from_this());
    loop_->removeFromPoller(channel_);
}

void HttpData::newEvent() {
    channel_->setEvents(DEFAULT_EVENT);
    loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);
}
