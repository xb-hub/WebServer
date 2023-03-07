//
// Created by 许斌 on 2021/10/27.
//

#ifndef _MYHTTP_H_
#define _MYHTTP_H_
#include <iostream>
#include <fstream>
#include <string.h>
#include <cstdlib>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <memory>
#include "ThreadPool.h"
#include "Scheduler.h"
#include "IOManager.h"

namespace xb
{
// 连接状态
enum ConnStatus
{
    SockError = -5,
    BindError = -4,
    ListenError = -3
};

// 主机状态：分析请求行，分析头部字段
enum CHECK_STATE
{
    CHECK_STATE_REQUESTLINE = 0,
    CHECK_STATE_HEADER = 1
};

enum LINE_STATUS
{
    LINE_OK = 0,    // 读取到完整行
    LINE_BAD = 1,   // 行出错
    LINE_OPEN = 2   // 行不完整
};

// HTTP报文状态
enum HttpStatus
{
    MethodError = -2,
    VersionError = -1,
    HttpSuccess = 0
};

// 文件类型
enum FileType
{
    Unknown = -1,
    Image = 1,
    Text = 2,
    Application = 3
};

struct File
{
    FileType type;
    std::string file_exten;
};

class MyHttp
{
private:
    std::string root_;  // 服务器文件根目录
    int thread_num_;    // 线程池线程数
    int serverfd_;      // 服务器套接字
    int port_;          // 端口

    ThreadPool::ptr pool;
    Scheduler::ptr schedule;
    // IOManager::ptr iomanager_;

protected:
    const int MAX_BUF_SIZE; // 缓冲区最大空间

public:
    MyHttp(const int port, const std::string& htdocs, int num);
    ~MyHttp();

    int start_up();
    std::string my_getline(int clientfd);
    int accept_request(int clientfd);
    void search_file(const std::string& path, int clientfd);
    void headers(const File& file_t, int clientfd);
    File FileType(const std::string& path);

    void execute_cgi(int clientfd, const std::string& path, const std::string& method, const std::string& paramter);

    // 错误页面返回
    void not_found(int clientfd);
    void client_error(int clientfd);
    void server_error(int clientfd);
    void forbidden(int clientfd);
    void bad_request(int clientfd);

};
}

#endif //_MYHTTP_H_