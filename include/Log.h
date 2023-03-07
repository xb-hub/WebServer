#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <list>
#include <memory>
#include <fstream>
#include <sstream>
#include "Singleton.h"
#include "Config.h"
#include "Thread.h"
#include "util.h"

#define GET_ROOT_LOGGER() xb::LoggerManager::getInstance()->getRoot()
#define GET_LOGGER(name) xb::LoggerManager::getInstance()->getLogger(name)

#define MAKE_EVENT(level, content) std::make_shared<xb::LogEvent>(__FILE__, __LINE__, 0, xb::GetThreadId(), 1, ::time(nullptr), content, level)

#define LOG_LEVEL(logger, level, content) logger->log(level, MAKE_EVENT(level, content))

#define LOG_DEBUG(logger, content) LOG_LEVEL(logger, xb::LogLevel::DEBUG, content)
#define LOG_INFO(logger, content) LOG_LEVEL(logger, xb::LogLevel::INFO, content)
#define LOG_WARN(logger, content) LOG_LEVEL(logger, xb::LogLevel::WARN, content)
#define LOG_ERROR(logger, content) LOG_LEVEL(logger, xb::LogLevel::ERROR, content)
#define LOG_FATAL(logger, content) LOG_LEVEL(logger, xb::LogLevel::FATAL, content)

#define LOG_FMT_LEVEL(logger, level, fmt, argv...)      \
    {                                                   \
        char *buf;                                      \
        int ret = asprintf(&buf, fmt, argv);            \
        if (ret != -1)                                  \
        {                                               \
            LOG_LEVEL(logger, level, std::string(buf)); \
            free(buf);                                  \
        }                                               \
    }
#define LOG_FMT_DEBUG(logger, fmt, argv...) LOG_FMT_LEVEL(logger, xb::LogLevel::DEBUG, fmt, argv)
#define LOG_FMT_INFO(logger, fmt, argv...) LOG_FMT_LEVEL(logger, xb::LogLevel::INFO, fmt, argv)
#define LOG_FMT_WARN(logger, fmt, argv...) LOG_FMT_LEVEL(logger, xb::LogLevel::WARN, fmt, argv)
#define LOG_FMT_ERROR(logger, fmt, argv...) LOG_FMT_LEVEL(logger, xb::LogLevel::ERROR, fmt, argv)
#define LOG_FMT_FATAL(logger, fmt, argv...) LOG_FMT_LEVEL(logger, xb::LogLevel::FATAL, fmt, argv)

namespace xb
{

    // 日志级别
    class LogLevel
    {
    public:
        enum Level
        {
            DEBUG = 0,
            INFO = 1,
            WARN = 2,
            ERROR = 3,
            FATAL = 4
        };

        static std::string Level2String(Level level);
    };

    // 日志事件
    class LogEvent
    {
    public:
        using ptr = std::shared_ptr<LogEvent>;
        LogEvent(const std::string &filename, int32_t line, uint32_t elapse,
                 uint32_t threadid, uint32_t fiberid, uint64_t time, const std::string &content, LogLevel::Level level);

        std::string getFilename() const { return m_filename; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadID() const { return m_threadid; }
        uint32_t getFiberID() const { return m_fiberid; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_content; }
        LogLevel::Level getLevel() const { return m_level; }

    private:
        std::string m_filename;  // 文件名
        int32_t m_line = 0;      // 行号
        uint32_t m_elapse = 0;   // 程序启动开始到现在的毫秒
        uint32_t m_threadid = 0; // 线程号
        uint32_t m_fiberid = 0;  // 协程号
        uint64_t m_time;         // 时间戳
        std::string m_content;   // 日志内容
        LogLevel::Level m_level;
    };

    // 日志格式器
    class LogFormatter
    {
    public:
        enum STR_STAUTS
        {
            COMMON_STR = 0,
            FORMAT_STR = 1
        };

    public:
        using ptr = std::shared_ptr<LogFormatter>;
        explicit LogFormatter(const std::string pattern);
        std::string format(LogEvent::ptr event);

        class FormatterItem
        {
        public:
            using ptr = std::shared_ptr<FormatterItem>;
            virtual void format(std::ostream &os, LogEvent::ptr event) = 0;
        };

    private:
        void init();

    private:
        std::string m_pattern;
        std::vector<FormatterItem::ptr> m_items_list;
    };

    // 日志输出地
    class LogAppender
    {
    public:
        using ptr = std::shared_ptr<LogAppender>;
        virtual ~LogAppender() = default;

        virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;
        LogFormatter::ptr getFormatter() const { return m_formatter; }
        void setFormatter(LogFormatter::ptr formatter) { m_formatter = formatter; }

    protected:
        LogFormatter::ptr m_formatter;
    };

    // 日志器
    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;
        Logger();
        Logger(const std::string &name, LogLevel::Level level);
        Logger(const std::string &name, LogLevel::Level level, const std::string &pattern);
        void log(LogLevel::Level level, LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void deleteAppender(LogAppender::ptr appender);

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }

    private:
        std::string m_name;                      // 日志名称
        LogLevel::Level m_level;                 // 日志级别
        const std::string m_formatter_pattern;   // 默认格式pattern
        LogFormatter::ptr m_formatter;           // 默认格式
        std::list<LogAppender::ptr> m_appenders; // Appender集合
        MutexLock mutex_;
    };

    // 日志输出到控制台
    class StdoutLogAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<StdoutLogAppender>;
        StdoutLogAppender() = default;
        void log(LogLevel::Level level, LogEvent::ptr event) override;

    private:
    };

    // 日志输出到文件
    class FileLogAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<FileLogAppender>;
        explicit FileLogAppender(const std::string &filename, LogLevel::Level level = LogLevel::DEBUG);
        void log(LogLevel::Level level, LogEvent::ptr event) override;
        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

    struct LogAppenderConfig
    {
        enum Type
        {
            Stdout = 0,
            File = 1
        };
        LogAppenderConfig::Type type;
        std::string file;
        std::string formatter;
        LogAppenderConfig() : type(LogAppenderConfig::Type::File)
        {
        }

        bool operator==(const LogAppenderConfig &rhs) const
        {
            return type == rhs.type && file == rhs.file;
        }

        friend std::ostream &operator<<(std::ostream &os, const LogAppenderConfig &config)
        {
            os << "file: " << config.file << "\nformatter: " << config.formatter << std::endl;
            return os;
        }
    };

    struct LogConfig
    {
        std::string name;
        LogLevel::Level level;
        std::vector<LogAppenderConfig> appender;

        LogConfig() = default;

        bool operator==(const LogConfig &rhs) const
        {
            return name == rhs.name;
        }
    };

    template <>
    class LexicalCast<std::string, std::vector<LogConfig>>
    {
    public:
        std::vector<LogConfig> operator()(const std::string &source)
        {
            YAML::Node node = YAML::Load(source);
            std::vector<LogConfig> config_list;
            for (const auto &log_config : node)
            {
                LogConfig lc;
                lc.name = log_config["name"] ? log_config["name"].as<std::string>() : "";
                lc.level = log_config["level"] ? static_cast<LogLevel::Level>(log_config["level"].as<int>()) : LogLevel::Level::DEBUG;
                if (log_config["appender"].IsDefined() && log_config["appender"].IsSequence())
                {
                    for (const auto &appender_config : log_config["appender"])
                    {
                        LogAppenderConfig lac;
                        lac.type = appender_config["type"] ? static_cast<LogAppenderConfig::Type>(appender_config["type"].as<int>()) : LogAppenderConfig::Type::File;
                        lac.file = appender_config["file"] ? appender_config["file"].as<std::string>() : "";
                        lac.formatter = appender_config["formatter"] ? appender_config["formatter"].as<std::string>() : "";
                        lc.appender.push_back(std::move(lac));
                    }
                }
                config_list.push_back(std::move(lc));
            }
            return config_list;
        }
    };

    template <>
    class LexicalCast<std::vector<LogConfig>, std::string>
    {
    public:
        std::string operator()(const std::vector<LogConfig> &config_list)
        {
            YAML::Node log_node;
            for (const auto &log_config : config_list)
            {
                log_node["name"] = log_config.name;
                log_node["level"] = static_cast<int>(log_config.level);
                YAML::Node app_list_node;
                for (const auto &appender_config : log_config.appender)
                {
                    YAML::Node app_node;
                    app_node["type"] = static_cast<int>(appender_config.type);
                    app_node["file"] = appender_config.file;
                    app_node["formatter"] = appender_config.formatter;
                    app_list_node.push_back(std::move(app_node));
                }
                log_node["appender"] = app_list_node;
            }
            std::stringstream ss;
            ss << log_node;
            return ss.str();
        }
    };

    class __LoggerManager
    {
    public:
        using ptr = std::shared_ptr<__LoggerManager>;
        __LoggerManager();
        Logger::ptr getLogger(const std::string &name);
        Logger::ptr getRoot();
        void init();
        void ensureRootLoggerExists();

    private:
        MutexLock m_mutex;
        std::map<std::string, Logger::ptr> m_logger_map;
    };

    using LoggerManager = SingletonPtr<__LoggerManager>;

    class LogIniter
    {
    public:
        LogIniter()
        {
            auto log_config_list = Config::LookUp<std::vector<LogConfig>>("logs", {}, "日志器的配置项");
            log_config_list->addListener([](const std::vector<LogConfig> &, const std::vector<LogConfig> &)
                                         {
            std::cout << "日志器配置变动，更新日志器" << std::endl;
            LoggerManager::getInstance()->init(); });
        }
    };
    static LogIniter __log_init__;

}
#endif
