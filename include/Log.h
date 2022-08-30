#ifdefine _LOG_H_
#define _LOG_H_
#include <string>
#include <list>
#include <memory>
#include <fstream>
#include <sstream>

namespace xb
{

// 日志级别
class LogLevel
{
public:
    enum Level
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };
};

// 日志事件
class LogEvent
{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent();

private:
    const char *m_file = nullptr;       // 文件名
    int32_t m_line = 0;                 // 行号
    uint32_t m_elapse = 0;              // 程序启动开始到现在的毫秒
    uint32_t m_threadid = 0;            // 线程号
    uint32_t fiberid = 0;               // 协程号
    uint64_t m_time;                    // 时间戳
    std::string m_content;              // 日志内容
};

// 日志格式器
class LogFormatter
{
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    std::string format(LogEvent::ptr event);

private:

};

// 日志输出地
class LogAppender
{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~logAppender() {}

    virtual void log(LogLevel::Level level, LogEvent::ptr event);
    LogFormatter::ptr getFormatter() { return m_formatter; }
private:
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
};

// 日志器
class Logger
{
public:
    Logger(const std::string& name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void deleteAppender(LogAppender::ptr appender);

    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level)    { m_level = level; }

private:
    std::string m_name;                         // 日志名称
    LogLevel::Level m_level;                    // 日志级别
    std::list<LogAppender::ptr> m_appenders;   // Appender集合
};

// 日志输出到控制台
class StdoutLogAppender : public LogAppender
{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    virtual void log(LogLevel::Level level, LogEvent::ptr event) override;

private:
};

// 日志输出到文件
class FileLogAppender : public LogAppender
{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(LogLevel::Level level, LogEvent::ptr event) override;
    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;

};

}
