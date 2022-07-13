#include "HttpRequest.h"
#include "../Log/Logger.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <regex>

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/welcome",
    "/video",
    "/picture",
    "/register",
    "/login",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

HttpRequest::HttpRequest() {
    _method = _path = _version = _body = "";
    _parseState = REQUEST_LINE;
    _header.clear();
    _post.clear();
    _keepAlive = false;
}

void HttpRequest::Init() {
    _method = _path = _version = _body = "";
    _parseState = REQUEST_LINE;
    _header.clear();
    _post.clear();
}

bool HttpRequest::ParseRequestLine(const std::string &line) {
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern)) {
        _method = subMatch[1];
        _path = subMatch[2];
        _version = subMatch[3];
        _parseState = HEADERS;
        return true;
    }
    LOG_ERROR("RequstLine Error", 0);
    return false;
}

void HttpRequest::ParsePath() {
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

void HttpRequest::ParseHeader(const std::string &line) {
    std::regex patten("([\\w-]+): (.*)");
    std::smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        _header[subMatch[1]] = subMatch[2];
    } else {
        _parseState = BODY;
    }
}

void HttpRequest::ParseFromUrlencoded() {
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
void HttpRequest::ParsePost() {
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

void HttpRequest::ParseBody(const std::string &line) {
    _body = line;
    ParsePost();
    _parseState = FINISH;
    LOG_TRACE("Body:%s, len:%d\n", line.c_str(), line.size());
}

bool HttpRequest::parse(std::string &buff) {
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

bool HttpRequest::isKeepAlive() const {
    return _keepAlive;
}

int HttpRequest::ConverHex(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}

std::string HttpRequest::path() const {
    return _path;
}

std::string &HttpRequest::path() {
    return _path;
}
std::string HttpRequest::method() const {
    return _method;
}

std::string HttpRequest::version() const {
    return _version;
}

std::string HttpRequest::GetPost(const std::string &key) const {
    assert(key != "");
    if (_post.count(key) == 1) {
        return _post.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const {
    assert(key != nullptr);
    if (_post.count(key) == 1) {
        return _post.find(key)->second;
    }
    return "";
}

HttpRequest::PARSE_STATE HttpRequest::GetParseState() {
    return _parseState;
}