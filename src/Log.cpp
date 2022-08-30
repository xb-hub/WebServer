//
// Created by 许斌 on 2022/5/6.
//
#include "Log.h"

namespace mywbeserver
{
Logger::Logger(const std::string& name = "root") :
        m_name(name)
{

}

void Logger::addAppender(LogAppender::ptr appender)
{
    m_appenders.push_back(appender);
}

void Logger::deleteAppender(LogAppender::ptr appender)
{
    for(auto it : m_appenders)
    {
        if(*it == appender)
        {
            my_appender.erase(it);
            break;
        }
    }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
        for(auto it : m_appenders)
        {
            it->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL, event);
}

void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
        std::cout << m_formatter->format(event);
    }
}

bool FileLogAppender::reopen()
{
    if(m_filestream)
    {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

FileLogAppender::FileLogAppender(const std::string &filename)
{
    if()
}

void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
        m_filestream << m_formatter->format(event);
    }
}
}