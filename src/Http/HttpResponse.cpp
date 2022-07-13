#include "HttpResponse.h"
#include "../Log/Logger.h"
#include <cassert>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
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

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() {
    _code = -1;
    _resErr = false;
    _path = _srcDir = "";
    _isKeepAlive = false;
    _mmFile = nullptr;
    _mmFileStat = {0};
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::Init(const std::string &srcDir, std::string &path, bool isKeepAlive, int code) {
    assert(srcDir != "");
    if (_mmFile) {
        UnmapFile();
    }
    _resErr = false;
    _code = code;
    _isKeepAlive = isKeepAlive;
    _path = path;
    _srcDir = srcDir;
    _mmFile = nullptr;
    _mmFileStat = {0};
}

char *HttpResponse::File() {
    return _mmFile;
}

size_t HttpResponse::FileLen() const {
    return _mmFileStat.st_size;
}

void HttpResponse::ErrorHtml() {
    if (CODE_PATH.count(_code) == 1) {
        _path = CODE_PATH.find(_code)->second;
        stat((_srcDir + _path).data(), &_mmFileStat);
    }
}

void HttpResponse::AddStateLine(std::string &buff) {
    std::string status;
    if (CODE_STATUS.count(_code) == 1) {
        status = CODE_STATUS.find(_code)->second;
    } else {
        _code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff += ("HTTP/1.1 " + std::to_string(_code) + " " + status + "\r\n");
}

void HttpResponse::AddHeader(std::string &buff) {
    buff += "Connection: ";
    if (_isKeepAlive) {
        buff += "keep-alive\r\n";
        buff += "keep-alive: max=6, timeout=120\r\n";
    } else {
        buff += "close\r\n";
    }
    buff += "Content-type: " + GetFileType() + "\r\n";
    buff += "Server: EasyServer\r\n";
}

void HttpResponse::AddContent(std::string &buff) {
    int srcFd = open((_srcDir + _path).data(), O_RDONLY);
    if (srcFd < 0) {
        _resErr = true;
        ErrorContent(buff, "File Not Found");
        return;
    }

    /* 将文件映射到内存提高文件的访问速度
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_TRACE("file path %s \n", (_srcDir + _path).data());
    int *mmRet = (int *)mmap(0, _mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    close(srcFd);
    if (*mmRet == -1) {
        _resErr = true;
        ErrorContent(buff, "File NotFound!");
        return;
    }
    _mmFile = (char *)mmRet;
    buff += "Content-length: " + std::to_string(_mmFileStat.st_size) + "\r\n\r\n";
    buff += std::string(_mmFile, _mmFile + _mmFileStat.st_size);
}

void HttpResponse::UnmapFile() {
    if (_mmFile) {
        munmap(_mmFile, _mmFileStat.st_size);
        _mmFile = nullptr;
    }
}

std::string HttpResponse::GetFileType() {
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

void HttpResponse::ErrorContent(std::string &buff, std::string message) {
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

void HttpResponse::MakeResponse(std::string &buff) {
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

void HttpResponse::setCode(int code) {
    _code = code;
}
