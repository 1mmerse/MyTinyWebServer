//
// Created by 杨成进 on 2020/7/27.
//

#ifndef MYTINYWEBSERVER_HTTPREQUEST_H
#define MYTINYWEBSERVER_HTTPREQUEST_H

#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <mysql/mysql.h>
#include "../buffer/Buffer.h"
#include "../log/Log.h"
#include "../pool/sqlconnRAII.h"

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
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERVAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest() {
        Init();
    }

    ~HttpRequest() = default;

    void Init();

    bool parse(Buffer &buff);

    std::string path() const;

    std::string &path();

    std::string method() const;

    std::string version() const;

    std::string GetPost(const std::string &key) const;

    std::string GetPost(const char *key) const;

    bool IsKeepAlive() const;

private:
    bool ParseRequestLine_(const std::string &line);

    void ParseHeader_(const std::string &line);

    void ParseBody_(const std::string &line);

    void ParsePath_();

    void ParsePost_();

    void ParseFromUrlEncode_();

    static bool UserVerify(const std::string &name, const std::string& password, bool isLogin);


    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    static int ConverHex(char ch);
};


#endif //MYTINYWEBSERVER_HTTPREQUEST_H
