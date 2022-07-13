#ifndef __SRC_HTTP_HTTPRESPONSE_H_
#define __SRC_HTTP_HTTPRESPONSE_H_

#include <string>
#include <sys/stat.h> // stat
#include <sys/mman.h> // mmap, munmap
#include <unordered_map>
class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);
    // void MakeResponse(Buffer &buff);
    void MakeResponse(std::string& buff);
    void handleErrorResponse(std::string& buff);
    void UnmapFile();
    char *File();
    size_t FileLen() const;
    void ErrorContent(std::string& buff, std::string messages);
    int Code() const {
        return _code;
    };
    void setCode(int code);
    bool resErr() {
        return _resErr;
    }

private:
    void AddStateLine(std::string &buff);
    void AddHeader(std::string &buff);
    void AddContent(std::string &buff);

    void ErrorHtml();
    std::string GetFileType();

    int _code;
    bool _resErr;
    bool _isKeepAlive;

    std::string _path;
    std::string _srcDir;

    char *_mmFile;
    struct stat _mmFileStat;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif /* __SRC_HTTP_HTTPRESPONSE_H_ */
