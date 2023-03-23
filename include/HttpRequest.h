#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_
#include <memory>
#include <string>
#include <regex>
#include <mysql/mysql.h>
#include <unordered_set>
#include <unordered_map>
#include "Buffer.h"
#include "Log.h"
// #include "sqlconnpool.h"
// #include "sqlconnRAII.h"

namespace xb
{
    class HttpRequest
    {
    public:
        enum PARSE_STATE
        {
            REQUEST_LINE = 0,
            HEADERS,
            BODY,
            FINISH
        };

        enum HTTP_STATE_CODE
        {
            NO_REQUEST = 0,
            GET_REQUEST,
            BAD_REQUEST,
            NO_RESOURSE,
            FORBIDDENT_REQUEST,
            FILE_REQUEST,
            INTERNAL_ERROR,
            CLOSED_CONNECTION,
        };

        HttpRequest() { Init(); }
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

        /*
        todo
        void HttpConn::ParseFormData() {}
        void HttpConn::ParseJson() {}
        */

    private:
        bool ParseRequestLine_(const std::string &line);
        void ParseHeader_(const std::string &line);
        void ParseBody_(const std::string &line);

        void ParsePath_();
        void ParsePost_();
        void ParseFromUrlencoded_();

        static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

        PARSE_STATE state_;
        std::string method_, path_, version_, body_;
        std::unordered_map<std::string, std::string> header_;
        std::unordered_map<std::string, std::string> post_;

        static const std::unordered_set<std::string> DEFAULT_HTML;
        static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
        static int ConverHex(char ch);

    }; 
} // namespace xb

#endif