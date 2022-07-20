#ifndef __SRC_HTTP_HTTPREQUEST_H_
#define __SRC_HTTP_HTTPREQUEST_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
class HttpRequest {
public:
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

    HttpRequest();
    ~HttpRequest() = default;

    void Init();

    // bool parse(Buffer &buff);
    bool parse(std::string& buff);

    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string &key) const;
    std::string GetPost(const char *key) const;
    PARSE_STATE GetParseState();

    bool isKeepAlive() const;

private:
    bool ParseRequestLine(const std::string &line);
    void ParseHeader(const std::string &line);
    void ParseBody(const std::string &line);

    void ParsePath();
    void ParsePost();
    void ParseFromUrlencoded();

    bool _keepAlive;
    PARSE_STATE _parseState;
    std::string _method, _path, _version, _body;
    std::unordered_map<std::string, std::string> _header;
    std::unordered_map<std::string, std::string> _post;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);
};

#endif /* __SRC_HTTP_HTTPREQUEST_H_ */
