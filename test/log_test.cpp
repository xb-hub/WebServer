//
// Created by 许斌 on 2022/9/7.
//

#include "Log.h"
using namespace xb;

int main()
{
    //    YAML::Node node = YAML::LoadFile("../config.yaml");
    //    Config::LoadFromYaml(node);
    //    std::cout << node["logs"][0]["appender"][0]["formatter"] << std::endl;
    //    std::shared_ptr<Logger> logger =  LoggerManager::getInstance()->getGlobal();
    // std::shared_ptr<Logger> logger = std::make_shared<Logger>("root", LogLevel::DEBUG, "[%p] [%d] [%t] [%f:%l]%T%m%n");
    // std::shared_ptr<LogEvent> event = std::make_shared<LogEvent>(LogLevel::DEBUG, __FILE__, __LINE__, 1111111, GetThreadId(), GetFiberId(), 1111111111, "test");
    // std::shared_ptr<LogAppender> appender = std::make_shared<StdoutLogAppender>();
    // logger->addAppender(appender);
    //    auto config = Config::LookUp("logs");
    LOG_DEBUG(GET_ROOT_LOGGER(), "DEBUG");
    //    LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "accetp request from : %d", 1)
    // logger->debug(event);
    //    LOG_DEBUG(GET_ROOT_LOGGER(), "DEBUG");
    // LOG_INFO(logger, "INFO");
    // LOG_WARN(logger, "warn");
    // LOG_ERROR(logger, "error");
    // LOG_FATAL(logger, "fatal");
    // LOG_FMT_DEBUG(logger, "消息 %s", "message");
    return 0;
}