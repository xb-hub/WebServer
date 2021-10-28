//
// Created by 许斌 on 2021/10/27.
//
#define _DEBUG_MESSAGE_
//#define _DEBUG_TYPE_
//#define _DEBUG_HEAD_

#include <thread>
#include "MyHttp/MyHttp.h"
using namespace xb;

MyHttp::MyHttp(const int port, const std::string htdocs, bool flag, int num) :
        serverfd_(-1),
        clientfd_(-1),
        root_(htdocs),  // "/Users/xubin/Desktop/MyHttp/htdocs"
        htaccess_path_("/Users/xubin/Desktop/MyHttp/htdocs/.htaccess"),
        thread_num_(num),
        port_(port),
        use_pool_(flag),
        MAX_BUF_SIZE(1024)
{
    pool = ThreadPool::getInstance(thread_num_, 20);
//    std::ifstream access(htaccess_path_, std::ios::in);
//    std::string ip;
//    while (!access.eof()) {
//        std::getline(access, ip);
//        std::string op = ip.substr(0, 4);
//        if(op == "Deny")
//        {
//            ip = ip.substr(10, std::string::npos);
//            deny[ip] = true;
//        }
//    }
}

MyHttp::~MyHttp() {
    close(serverfd_);
}

int MyHttp::start_up() {
    if((serverfd_ = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        return SockError;
    }
    struct sockaddr_in addr = {0};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(serverfd_, (struct sockaddr*)(&addr), sizeof(addr)) < 0) {
        return BindError;
    }
    if(listen(serverfd_, 5) < 0) {
        return ListenError;
    }

    struct  sockaddr_in client_addr = {0};
    socklen_t len = sizeof(client_addr);
    while (true) {
        if ((clientfd_ = accept(serverfd_, (struct sockaddr *) (&client_addr), &len)) < 0) {
            continue;
        }
//        if(deny[inet_ntoa(client_addr.sin_addr)]) {
//            std::cout << "deny from " << inet_ntoa(client_addr.sin_addr) << std::endl;
//            forbidden();
//            close(clientfd_);
//            continue;
//        }
        if(use_pool_) {
            pool->AddTask(std::bind(&MyHttp::accept_request, this));
        } else {
            std::thread t(&MyHttp::accept_request, this);
            t.detach();
        }
    }
}

std::string MyHttp::my_getline() const {
    int index = 0;
    char c = '\0';
    std::string line;
    while (index < MAX_BUF_SIZE && c != '\n') {
        if (recv(clientfd_, &c, 1, 0) > 0) {
            if (c == '\r') {
                if (recv(clientfd_, &c, 1, MSG_PEEK) > 0) {
                    if(c == '\n') {
                        recv(clientfd_, &c, 1, 0);
                    } else {
                        c = '\n';
                    }
                }
            }
            index++;
            line += c;
        }
    }
    return line;
}
int MyHttp::accept_request() {
    std::string head;
    std::string method;
    std::string url;
    std::string version;
    head = my_getline();
#ifdef _DEBUG_HEAD_
    std::cout << head << std::endl;
#endif
    int index = 0;
    while (index < head.size() && !isspace(head[index]))
    {
        method += head[index++];
    }
#ifdef _DEBUG_HEAD_
    std::cout << method << std::endl;
#endif
    if(strcasecmp(method.c_str(), "GET") != 0) return MethodError;
    while (isspace(head[index]))  index++;
    while (index < head.size() && !isspace(head[index]))
    {
        url += head[index++];
    }
#ifdef _DEBUG_HEAD_
    std::cout << url << std::endl;
#endif
    while (isspace(head[index]))  index++;
    while (index < head.size() && head[index] != '\n')
    {
        version += head[index++];
    }
#ifdef _DEBUG_HEAD_
    std::cout << version << std::endl;
#endif
    if(strcasecmp(version.c_str(), "HTTP/1.1") != 0)   return VersionError;
    std::string path = root_ + url;
    if(path.back() == '/') {
        path += "index.html";
    }
#ifdef _DEBUG_HEAD_
    std::cout << path << std::endl;
#endif

    int len = 1;
    std::string message;
    struct stat st;
    if(stat(path.c_str(), &st) == -1) {
        while (len > 0 && message != "\n") {
            message = my_getline();
#ifdef _DEBUG_MESSAGE_
            std::cout << message << std::endl;
#endif
            len = message.size();
        }
        not_found();
    }
    else {
        if(S_ISDIR(st.st_mode))     path += "/index.html";    // 如果路径所指是目录文件
        search_file(path);
    }
    close(clientfd_);
    return HttpSuccess;
}

void MyHttp::headers(File file_t) const
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    switch (file_t.type) {
        case Photo:
            response += "Content-Type: image/";
            response += file_t.file_exten;
            response += " \r\n";
            break;
        case Html:
            response += "Content-Type: text/";
            response += file_t.file_exten;
            response += " \r\n";
            break;
        default:
            response += "Content-Type: text/html \r\n";
    }
    response += "\r\n";
    send(clientfd_, response.c_str(), response.size(), 0);
}

void MyHttp::search_file(const std::string& path) {
    int len = 1;
    std::string message;
    std::string file_content;
    while (len > 0 && message != "\n") {
        message = my_getline();
#ifdef _DEBUG_MESSAGE_
        std::cout << message;
#endif
        len = message.size();
    }
    File file_t = FileType(path);
#ifdef _DEBUG_TYPE_
    std::cout << file_t.type << std::endl;
#endif
    headers(file_t);
    std::ifstream file;
    switch (file_t.type) {
        case Html: {
            file.open(path);
            std::string tmp;
            while (!file.eof()) {
                std::getline(file, tmp);
                file_content += tmp;
                file_content += "\n";
            }
#ifdef _DEBUG_MESSAGE_
            std::cout << file_content << std::endl;
#endif
            send(clientfd_, file_content.c_str(), file_content.size(), 0);
            break;
        }
        case Photo: {
            file.open(path, std::ifstream::in | std::ios::binary);
            file.seekg(0, file.end);
            int length = file.tellg();
            file.seekg(0, file.beg);
            char * buffer = new char[length];
            file.read(buffer, length);
            send(clientfd_, buffer, length, 0);
            break;
        }
        case Unknown:
#ifdef _DEBUG_TYPE_
            std::cout << "Unknow" << std::endl;
#endif
            break;
    }
    file.close();
}

void MyHttp::not_found() {
    std::ifstream file("/Users/xubin/Desktop/MyHttp/htdocs/404.html");
    std::string response = "HTTP/1.1 404 Not Found\r\n";
    response += "\r\n";
    std::string tmp;
    while (!file.eof()) {
        std::getline(file, tmp);
        response += tmp;
        response += "\n";
    }
    send(clientfd_, response.c_str(), response.size(), 0);
    file.close();
}

void MyHttp::forbidden() {
    std::ifstream file("/Users/xubin/Desktop/MyHttp/htdocs/403.html");
    std::string response = "HTTP/1.1 403 Forbidden\r\n";
    response += "\r\n";
    std::string tmp;
    while (!file.eof()) {
        std::getline(file, tmp);
        response += tmp;
        response += "\n";
    }
    send(clientfd_, response.c_str(), response.size(), 0);
    file.close();
}

void MyHttp::client_error() {
#ifdef _DEBUG_MESSAGE_
    std::cout << "400 client error" << std::endl;
#endif
}

void MyHttp::server_error() {
#ifdef _DEBUG_MESSAGE_
    std::cout << "500 server error" << std::endl;
#endif
}

File MyHttp::FileType(const std::string &path) {
    size_t pos= path.rfind('.');
    if (pos == std::string::npos) {
        return {Unknown, ""};
    }
    std::string file_exten = path.substr(pos + 1, std::string::npos);
    if(file_exten == "jpg") file_exten = "jpeg";
    if (!strcasecmp(file_exten.c_str(), "jpeg") || !strcasecmp(file_exten.c_str(), "png") || !strcasecmp(file_exten.c_str(), "gif")) {
        return {Photo, file_exten};
    }
    else if(!strcasecmp(file_exten.c_str(), "htm") || !strcasecmp(file_exten.c_str(), "html")) {
        return {Html, file_exten};
    }
    return {Unknown, ""};
}