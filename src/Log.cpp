//
// Created by 许斌 on 2022/5/6.
//
#include "Log.h"
#include <map>

namespace xb
{
    /*********************** LogLevel ***********************/
    std::string LogLevel::Level2String(Level level)
    {
        std::string result;
        switch (level)
        {
        case Level::DEBUG:
            result = "DEBUG";
            break;
        case Level::INFO:
            result = "INFO";
            break;
        case Level::WARN:
            result = "WARN";
            break;
        case Level::ERROR:
            result = "ERROR";
            break;
        case Level::FATAL:
            result = "FATAL";
        }
        return result;
    }

    /*********************** LogEvent ***********************/
    LogEvent::LogEvent(const std::string &filename, int32_t line, uint32_t elapse, uint32_t threadid, uint32_t fiberid,
                       uint64_t time, const std::string &content, LogLevel::Level level) : m_filename(filename),
                                                                                           m_line(line),
                                                                                           m_elapse(elapse),
                                                                                           m_threadid(threadid),
                                                                                           m_fiberid(fiberid),
                                                                                           m_time(time),
                                                                                           m_content(content),
                                                                                           m_level(level)
    {
    }

    /*********************** Logger ***********************/
    Logger::Logger() : m_name("default"),
                       m_level(LogLevel::Level::DEBUG),
                       m_formatter_pattern("[%d] [%p] [%f:%l]%T [%t] %m%n")
    {
        m_formatter.reset(new LogFormatter(m_formatter_pattern));
    }

    Logger::Logger(const std::string &name, LogLevel::Level level) : m_name(name),
                                                                     m_level(level),
                                                                     m_formatter_pattern("[%d] [%p] [%f:%l]%T [%t] %m%n")
    {
        m_formatter.reset(new LogFormatter(m_formatter_pattern));
    }

    Logger::Logger(const std::string &name, LogLevel::Level level, const std::string &pattern) : m_name(name),
                                                                                                 m_level(level),
                                                                                                 m_formatter_pattern(pattern)
    {
        m_formatter.reset(new LogFormatter(m_formatter_pattern));
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        if (!appender->getFormatter())
        {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    void Logger::deleteAppender(LogAppender::ptr appender)
    {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            for (auto it : m_appenders)
            {
                it->log(level, event);
            }
        }
    }

    /*********************** Appender ***********************/
    void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event)
    {
        std::cout << m_formatter->format(event);
    }

    bool FileLogAppender::reopen()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename, std::ios_base::out | std::ios_base::app);
        return !!m_filestream;
    }

    FileLogAppender::FileLogAppender(const std::string &filename, LogLevel::Level level) : m_filename(filename)
    {
        reopen();
    }

    void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event)
    {
        m_filestream << m_formatter->format(event);
        m_filestream.flush();
    }

    /*********************** Formatter ***********************/
    class CommonStrFormatItem : public LogFormatter::FormatterItem
    {
    public:
        explicit CommonStrFormatItem(const std::string &str) : m_str(str)
        {
        }
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << m_str;
        }

    private:
        std::string m_str;
    };

    class LevelFormatItem : public LogFormatter::FormatterItem
    {
    public:
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << LogLevel::Level2String(event->getLevel());
        }
    };

    class FilenameFormatItem : public LogFormatter::FormatterItem
    {
    public:
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFilename();
        }
    };

    class TimeFormatItem : public LevelFormatItem::FormatterItem
    {
    public:
        explicit TimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") : m_format(format)
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            struct tm local;
            time_t time = event->getTime();
            localtime_r(&time, &local);
            char time_buf[64];
            strftime(time_buf, sizeof(time_buf), m_format.c_str(), &local);
            os << time_buf;
        }

    private:
        std::string m_format;
    };

    class ThreadIdFormatItem : public LogFormatter::FormatterItem
    {
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getThreadID();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatterItem
    {
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFiberID();
        }
    };

    class ContentFormatItem : public LogFormatter::FormatterItem
    {
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };

    class LineFormatItem : public LogFormatter::FormatterItem
    {
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatterItem
    {
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "\n";
        }
    };

    class PercentSignFormatItem : public LogFormatter::FormatterItem
    {
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "%";
        }
    };

    class TabFormatItem : public LogFormatter::FormatterItem
    {
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "\t";
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatterItem
    {
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };

    LogFormatter::LogFormatter(const std::string pattern) : m_pattern(pattern)
    {
        init();
    }

    void LogFormatter::init()
    {
        static std::map<char, LogFormatter::FormatterItem::ptr> m_format_items_map = {
#define FN(CN, ITEM_NAME) {CN, std::make_shared<ITEM_NAME>()}
            FN('p', LevelFormatItem),
            FN('f', FilenameFormatItem),
            FN('d', TimeFormatItem),
            FN('t', ThreadIdFormatItem),
            FN('f', FiberIdFormatItem),
            FN('m', ContentFormatItem),
            FN('l', LineFormatItem),
            FN('n', NewLineFormatItem),
            FN('%', PercentSignFormatItem),
            FN('T', TabFormatItem),
            FN('r', ElapseFormatItem)
#undef FN
        };
        STR_STAUTS stauts = COMMON_STR;
        for (size_t i = 0; i < m_pattern.size(); i++)
        {
            std::string str;
            switch (stauts)
            {
            case COMMON_STR:
            {
                while (i < m_pattern.size() && m_pattern[i] != '%')
                {
                    str += m_pattern[i];
                    i++;
                }
                stauts = FORMAT_STR;
                m_items_list.push_back(std::make_shared<CommonStrFormatItem>(str));
                break;
            }
            case FORMAT_STR:
            {
                if (m_format_items_map.find(m_pattern[i]) == m_format_items_map.end())
                {
                    m_items_list.push_back(std::make_shared<CommonStrFormatItem>("error format!"));
                }
                else
                {
                    m_items_list.push_back(m_format_items_map[m_pattern[i]]);
                }
                stauts = COMMON_STR;
                break;
            }
            }
        }
    }

    std::string LogFormatter::format(LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto it : m_items_list)
        {
            it->format(ss, event);
        }
        return ss.str();
    }

    __LoggerManager::__LoggerManager()
    {
        init();
    }

    void __LoggerManager::init()
    {
        MutexLockGuard lock(m_mutex);
        auto config = Config::LookUp<std::vector<LogConfig>>("logs");
        auto config_log_list = config->getValue();
        for (const auto &log_config : config_log_list)
        {
            // m_logger_map.erase(log_config.name);
            auto logger = std::make_shared<Logger>(log_config.name, log_config.level);
            for (const auto &appender_config : log_config.appender)
            {
                LogAppender::ptr appender;
                switch (appender_config.type)
                {
                case LogAppenderConfig::Type::Stdout:
                    appender = std::make_shared<StdoutLogAppender>();
                    break;
                case LogAppenderConfig::Type::File:
                    appender = std::make_shared<FileLogAppender>(appender_config.file);
                    break;
                default:
                    std::cerr << "LoggerManager::init exception 无效的 appender" << std::endl;
                    break;
                }
                appender->setFormatter(std::make_shared<LogFormatter>(appender_config.formatter));
                logger->addAppender(std::move(appender));
            }
            // std::cout << "成功创建日志器 " << log_config.name << std::endl;
            m_logger_map.insert(std::make_pair(log_config.name, std::move(logger)));
        }
        ensureRootLoggerExists();
    }

    void __LoggerManager::ensureRootLoggerExists()
    {
        auto it = m_logger_map.find("root");
        if (it == m_logger_map.end())
        {
            auto logger = std::make_shared<Logger>();
            logger->addAppender(std::make_shared<StdoutLogAppender>());
            m_logger_map.insert(std::make_pair("root", logger));
        }
        else if (!it->second)
        {
            it->second = std::make_shared<Logger>();
            it->second->addAppender(std::make_shared<StdoutLogAppender>());
        }
    }

    typename Logger::ptr __LoggerManager::getLogger(const std::string &name)
    {
        auto iter = m_logger_map.find(name);
        if (iter == m_logger_map.end())
        {
            return m_logger_map.find("root")->second;
        }
        return iter->second;
    }

    typename Logger::ptr __LoggerManager::getRoot()
    {
        return getLogger("root");
    }

}