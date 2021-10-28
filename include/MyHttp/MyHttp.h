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

namespace xb {
enum ConnStatus {
    SockError = -5,
    BindError = -4,
    ListenError = -3,
};

enum HttpStatus {
    MethodError = -2,
    VersionError = -1,
    HttpSuccess = 0
};

enum FileType{
    Unknown = -1,
    Photo = 1,
    Html = 2,
};

struct File {
    FileType type;
    std::string file_exten;
};

class MyHttp {
private:
    std::string root_;
    std::string htaccess_path_;
    int thread_num_;
    int serverfd_;
    int clientfd_;
    int port_;
//    std::unordered_map<std::string, bool> deny;
//    std::unordered_map<std::string, bool> allow;

    ThreadPool* pool;

    bool use_pool_;

protected:
    const int MAX_BUF_SIZE;

public:
    MyHttp(const int port, const std::string htdocs, bool flag, int num);
    ~MyHttp();

    int start_up();
    std::string my_getline() const;
    int accept_request();
    void search_file(const std::string& path);
    File FileType(const std::string& path);

    // response
    void not_found();
    void client_error();
    void server_error();
    void forbidden();

    void headers(File file_t) const;
};
}

#endif //THREADPOOL_MYHTTP_H