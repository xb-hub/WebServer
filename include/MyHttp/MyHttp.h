//
// Created by 许斌 on 2021/10/27.
//

#ifndef THREADPOOL_MYHTTP_H
#define THREADPOOL_MYHTTP_H
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
//    std::string htaccess_path_;
    std::string root_;  // 服务器文件根目录
    int thread_num_;    // 线程池线程数
    int serverfd_;      // 服务器套接字
    int port_;          // 端口
//    std::unordered_map<std::string, bool> deny;
//    std::unordered_map<std::string, bool> allow;

    // ThreadPool* pool;   // 线程池实例
    std::shared_ptr<ThreadPool> pool;

    bool use_pool_;     // 是否使用线程池

protected:
    const int MAX_BUF_SIZE; // 缓冲区最大空间

public:
    pthread_mutex_t buf_lock;

    MyHttp(const int port, const std::string& htdocs, bool flag, int num);
    ~MyHttp();

    int start_up();
    std::string my_getline(int clientfd);
    int accept_request(int clientfd);
    void search_file(const std::string& path, int clientfd);
    void headers(File file_t, int clientfd) const;
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

#endif //THREADPOOL_MYHTTP_H