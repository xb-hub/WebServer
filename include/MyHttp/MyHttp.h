//
// Created by 许斌 on 2021/10/27.
//

#ifndef THREADPOOL_MYHTTP_H
#define THREADPOOL_MYHTTP_H
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include "ThreadPool/ThreadPool.h"

namespace xb
{
// 连接状态
enum ConnStatus
{
    SockError = -5,
    BindError = -4,
    ListenError = -3,
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
    Photo = 1,
    Html = 2,
};

struct File
{
    FileType type;
    std::string file_exten;
};

class MyHttp
{
private:
    std::string htaccess_path_;
    std::string root_;  // 服务器文件根目录
    int thread_num_;    // 线程池线程数
    int serverfd_;      // 服务器套接字
    int clientfd_;      // 客户端套接字
    int port_;          // 端口
//    std::unordered_map<std::string, bool> deny;
//    std::unordered_map<std::string, bool> allow;

    ThreadPool* pool;   // 线程池实例

    bool use_pool_;     // 是否使用线程池

protected:
    const int MAX_BUF_SIZE; // 缓冲区最大空间

public:
    MyHttp(const int port, const std::string& htdocs, bool flag, int num);
    ~MyHttp();

    int start_up();
    std::string my_getline();
    int accept_request();
    void search_file(const std::string& path);
    void headers(File file_t) const;
    File FileType(const std::string& path);

    // 错误页面返回
    void not_found();
    void client_error();
    void server_error();
    void forbidden();

};
}

#endif //THREADPOOL_MYHTTP_H