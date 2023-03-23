//
// Created by 许斌 on 2022/9/9.
//

#ifndef _UTIL_H_
#define _UTIL_H_

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <stdint.h>
#include <json/json.h>
#include <yaml-cpp/yaml.h>

namespace xb
{

    pid_t GetThreadId();

    uint64_t GetCurrentMS();

    uint64_t GetCurrentUS();

    template <class T>
    const char *TypeToName();

    class StringUtil
    {
    public:
        static std::string Format(const char *fmt, ...);
        static std::string Formatv(const char *fmt, va_list ap);

        static std::string UrlEncode(const std::string &str, bool space_as_plus = true);
        static std::string UrlDecode(const std::string &str, bool space_as_plus = true);

        static std::string Trim(const std::string &str, const std::string &delimit = " \t\r\n");
        static std::string TrimLeft(const std::string &str, const std::string &delimit = " \t\r\n");
        static std::string TrimRight(const std::string &str, const std::string &delimit = " \t\r\n");

        static std::string WStringToString(const std::wstring &ws);
        static std::wstring StringToWString(const std::string &s);
    };

    class TypeUtil
    {
    public:
        static int8_t ToChar(const std::string &str);
        static int64_t Atoi(const std::string &str);
        static double Atof(const std::string &str);
        static int8_t ToChar(const char *str);
        static int64_t Atoi(const char *str);
        static double Atof(const char *str);
    };

    bool YamlToJson(const YAML::Node &ynode, Json::Value &jnode);
    bool JsonToYaml(const Json::Value &jnode, YAML::Node &ynode);

    class JsonUtil
    {
    public:
        static bool NeedEscape(const std::string &v);
        static std::string Escape(const std::string &v);
        static std::string GetString(const Json::Value &json, const std::string &name, const std::string &default_value = "");
        static double GetDouble(const Json::Value &json, const std::string &name, double default_value = 0);
        static int32_t GetInt32(const Json::Value &json, const std::string &name, int32_t default_value = 0);
        static uint32_t GetUint32(const Json::Value &json, const std::string &name, uint32_t default_value = 0);
        static int64_t GetInt64(const Json::Value &json, const std::string &name, int64_t default_value = 0);
        static uint64_t GetUint64(const Json::Value &json, const std::string &name, uint64_t default_value = 0);
        static bool FromString(Json::Value &json, const std::string &v);
        static std::string ToString(const Json::Value &json);
    };

}

#endif //_UTIL_H_
