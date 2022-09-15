//
// Created by 许斌 on 2021/10/27.
//
//#define _DEBUG_
//#define _DEBUG_MESSAGE_
//#define _DEBUG_TYPE_
//#define _DEBUG_HEAD_

#include <thread>
#include <poll.h>
#include <sys/select.h>
#include "MyHttp.h"
#include "Log.h"
#include "util.h"
using namespace xb;

// 构造函数初始化
MyHttp::MyHttp(const int port, const std::string& htdocs, bool flag, int num) :
        serverfd_(-1),
        root_(htdocs),  // "/Users/xubin/Desktop/MyHttp/htdocs"
//        htaccess_path_("/Users/xubin/Desktop/MyHttp/htdocs/.htaccess"),
        thread_num_(num),
        port_(port),
        use_pool_(flag),
        MAX_BUF_SIZE(1024)
{
    pool = ThreadPoolMgr::getInstance(thread_num_, 20);
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

// 析构函数
MyHttp::~MyHttp()
{
    if(pool)    pool->stop();
    close(serverfd_);
}

// 启动服务器
int MyHttp::start_up()
{
    int on = 1;
    if((serverfd_ = socket(PF_INET, SOCK_STREAM, 0)) < 0)   // 创建套接字
        return SockError;
    /**
     * setsockopt选项
     * SO_REUSEADDR     避免TIME_WAIT状态
     * SO_RCVBUF SO_SNDBUF  接收缓冲区和发送缓冲区大小
     * SO_RCVLOWAT SO_SNDLOWAT  接收缓冲区和发送缓冲区低水位标志
     */
    setsockopt(serverfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct timeval time = {5, 0};
    setsockopt(serverfd_, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(struct timeval));


    struct sockaddr_in addr = {0};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(serverfd_, (struct sockaddr*)(&addr), sizeof(addr)) < 0)
        return BindError;
    if(listen(serverfd_, 10) < 0)
        return ListenError;

    struct  sockaddr_in client_addr = {0};
    socklen_t len = sizeof(client_addr);

    size_t i;
    struct pollfd fds[MAX_BUF_SIZE];
    for(i = 1; i < MAX_BUF_SIZE; ++i)
    {
        fds[i].fd = -1;
    }
    nfds_t maxi = 0;
    fds[0].fd = serverfd_;
    fds[0].events = POLLIN;
    int clientfd, sockfd;

    while (true)
    {
        int n_ready = poll(fds, maxi + 1, -1);
        if (fds[0].revents & POLLIN)
        {
            clientfd = accept(serverfd_, (struct sockaddr *) (&client_addr), &len);
            LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "accetp request from : %d", clientfd);
            for (i = 1; i < MAX_BUF_SIZE; i++)
            {
                if (fds[i].fd < 0)
                {
                    fds[i].fd = clientfd;
                    break;
                }
            }
            if (i == MAX_BUF_SIZE)
            {
                LOG_DEBUG(GET_ROOT_LOGGER(), "too many clients\n");
                break;
            }
            fds[i].events = POLLIN;
            if (i > maxi)
            {
                maxi = i;
            }
            if (--n_ready <= 0)
            {
                continue;
            }
        }
        for (size_t i = 1; i <= maxi; i++)
        {
            sockfd = fds[i].fd;
            if (sockfd < 0) continue;
            if (fds[i].revents & (POLLIN | POLLERR))
            {
                if (use_pool_)   // 使用线程池
                    pool->AddTask(std::bind(&MyHttp::accept_request, this, sockfd));
                else    // 每次连接创建一个新线程
                {
                    std::thread t(&MyHttp::accept_request, this, sockfd);
                    t.join();
                }
            }
            fds[i].fd = -1;
            if(--n_ready <= 0)
                break;
        }
    }

//    while (true)
//    {
//        int clientfd = -1;
//        if ((clientfd = accept(serverfd_, (struct sockaddr *) (&client_addr), &len)) < 0)  continue;
////        if(deny[inet_ntoa(client_addr.sin_addr)]) {
////            std::cout << "deny from " << inet_ntoa(client_addr.sin_addr) << std::endl;
////            forbidden();
////            close(clientfd_);
////            continue;
////        }
//        LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "accetp request from : %d", clientfd);
//        if(use_pool_)   // 使用线程池
//            pool->AddTask(std::bind(&MyHttp::accept_request, this, clientfd));
//        else    // 每次连接创建一个新线程
//        {
//            std::thread t(&MyHttp::accept_request, this, clientfd);
//            t.join();
//        }
//    }
}

// 获取http报文的一行
std::string MyHttp::my_getline(int clientfd)
{
    int index = 0;
    char c = '\0';
    std::string line;
    while (index < MAX_BUF_SIZE && c != '\n')
    {
        if (recv(clientfd, &c, 1, 0) > 0)
        {
            if (c == '\r') {
                if (recv(clientfd, &c, 1, MSG_PEEK) > 0)
                {
                    if(c == '\n')
                        recv(clientfd, &c, 1, 0);
                    else
                        c = '\n';
                }
            }
            index++;
            line += c;
        }
    }
    return line;
}

// 处理http报文，获取method，url，version
int MyHttp::accept_request(int clientfd)
{
    std::string head;
    std::string method;
    std::string url;
    std::string version;
    head = my_getline(clientfd);
#ifdef _DEBUG_HEAD_
    std::cout << head << std::endl;
#endif
    int index = 0;
    while (index < head.size() && !isspace(head[index]))
        method += head[index++];
//    if(strcasecmp(method.c_str(), "GET") != 0) return MethodError;
    while (isspace(head[index]))  index++;
    while (index < head.size() && !isspace(head[index]))
        url += head[index++];
    while (isspace(head[index]))  index++;
    while (index < head.size() && head[index] != '\n')
        version += head[index++];
    if(strcasecmp(version.c_str(), "HTTP/1.1") != 0)   return VersionError;
    std::string path = root_ + url;
    if(path.back() == '/')
        path += strcasecmp(method.c_str(), "GET") ? "index.php" : "index.html";
#ifdef _DEBUG_MESSAGE_
    std::cout << path << std::endl;
#endif
    int len = 1;
    std::string message;
    struct stat st = {0};
    if(stat(path.c_str(), &st) == -1)
    {
        while (len > 0 && message != "\n")
        {
            message = my_getline(clientfd);
#ifdef _DEBUG_MESSAGE_
            std::cout << message << std::endl;
#endif
            len = message.size();
        }
        not_found(clientfd);
    }
    else
    {
        if(S_ISDIR(st.st_mode))     path += "/index.html";    // 如果路径所指是目录文件
        search_file(path, clientfd);
    }
    close(clientfd);
    return HttpSuccess;
}

void MyHttp::execute_cgi(int clientfd, const std::string& path, const std::string& method, const std::string& paramter)
{
    int len = 1;
    std::string message;
    int content_length = -1;
    if(strcasecmp(method.c_str(), "GET") == 0)
    {
        while (len > 0 && message != "\n")
        {
            message = my_getline(clientfd);
#ifdef _DEBUG_MESSAGE_
            std::cout << message << std::endl;
#endif
            len = message.size();
        }
    }
    else if(strcasecmp(method.c_str(), "POST") == 0)
    {
        message = my_getline(clientfd);
        len = message.size();
        while (len > 0 && message != "\n")
        {
            if(strcasecmp(message.substr(0, 15).c_str(), "Content-Length:"))
                content_length = stoi(message.substr(16, message.size() - 16));

            message = my_getline(clientfd);
#ifdef _DEBUG_MESSAGE_
            std::cout << message << std::endl;
#endif
            len = message.size();
        }
        if (content_length == -1) {
            bad_request(clientfd);
            return;
        }
    }
}

// 添加http响应报文头并区分文件类型
void MyHttp::headers(const File& file_t, int clientfd)
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    switch (file_t.type)
    {
        case Image:
            response += "Content-Type: image/";
            response += file_t.file_exten;
            response += " \r\n";
            break;
        case Text:
            response += "Content-Type: text/";
            response += file_t.file_exten;
            response += " \r\n";
            break;
        case Application:
            response += "Content-Type: application/x-javascript\r\n";
            break;
        default:
            response += "Content-Type: text/html \r\n";
    }
    response += "\r\n";
    send(clientfd, response.c_str(), response.size(), 0);
}

// 根据root路径和url路径寻找文件并通过套接字传输到客户端
void MyHttp::search_file(const std::string& path, int clientfd)
{
    int len = 1;
    std::string message;
    std::string file_content;
    while (len > 0 && message != "\n")
    {
        message = my_getline(clientfd);
#ifdef _DEBUG_MESSAGE_
        std::cout << message << std::endl;
#endif
        len = message.size();
    }
    File file_t = FileType(path);
#ifdef _DEBUG_TYPE_
    std::cout << file_t.type << std::endl;
#endif
    headers(file_t, clientfd);
    std::ifstream file;
    switch (file_t.type)
    {
        case Application:
        case Text:
        {
            file.open(path);
            std::string tmp;
            while (!file.eof()) {
                std::getline(file, tmp);
                file_content += tmp;
                file_content += "\r\n";
            }
#ifdef _DEBUG_MESSAGE_
            std::cout << file_content << std::endl;
#endif
            send(clientfd, file_content.c_str(), file_content.size(), 0);
            break;
        }
        case Image:
        {
            file.open(path, std::ifstream::in | std::ios::binary);
            file.seekg(0, file.end);
            int length = file.tellg();
            file.seekg(0, file.beg);
            char* buffer = new char[length];
            file.read(buffer, length);
            send(clientfd, buffer, length, 0);
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

// 404错误
void MyHttp::not_found(int clientfd)
{
#ifdef _DEBUG_MESSAGE_
    std::cout << "400 not found" << std::endl;
#endif
//    std::ifstream file("/Users/xubin/Desktop/MyHttp/htdocs/404.html");
//    std::string response = "HTTP/1.1 404 Not Found\r\n";
//    response += "Content-Type: text/html\r\n";
//    response += "\r\n";
//    std::string tmp;
//    while (!file.eof())
//    {
//        std::getline(file, tmp);
//        response += tmp;
//        response += "\r\n";
//    }
//#ifdef _DEBUG_MESSAGE_
//    std::cout << response << std::endl;
//#endif
//    send(clientfd_, response.c_str(), response.size(), 0);
//    file.close();
}

void MyHttp::bad_request(int clientfd)
{

}

// 403错误
void MyHttp::forbidden(int clientfd)
{
#ifdef _DEBUG_MESSAGE_
    std::cout << "403 Forbidden" << std::endl;
#endif
//    std::ifstream file("/Users/xubin/Desktop/MyHttp/htdocs/403.html");
//    std::string response = "HTTP/1.1 403 Forbidden\r\n";
//    response += "Content-Type: text/html \r\n";
//    response += "\r\n";
//    std::string tmp;
//    while (!file.eof())
//    {
//        std::getline(file, tmp);
//        response += tmp;
//        response += "\r\n";
//    }
//    send(clientfd_, response.c_str(), response.size(), 0);
//    file.close();
}

void MyHttp::client_error(int clientfd)
{
#ifdef _DEBUG_MESSAGE_
    std::cout << "400 client error" << std::endl;
#endif
}

void MyHttp::server_error(int clientfd)
{
#ifdef _DEBUG_MESSAGE_
    std::cout << "500 server error" << std::endl;
#endif
}

// 判断请求文件类型
File MyHttp::FileType(const std::string &path)
{
    size_t pos= path.rfind('.');
    if (pos == std::string::npos)
        return {Unknown, ""};
    std::string file_exten = path.substr(pos + 1, std::string::npos);
    if(file_exten == "jpg") file_exten = "jpeg";
    if (!strcasecmp(file_exten.c_str(), "jpeg") || !strcasecmp(file_exten.c_str(), "png") || !strcasecmp(file_exten.c_str(), "gif"))
        return {Image, file_exten};
    else if(!strcasecmp(file_exten.c_str(), "htm") || !strcasecmp(file_exten.c_str(), "html") || !strcasecmp(file_exten.c_str(), "css"))
        return {Text, file_exten};
    else if (!strcasecmp(file_exten.c_str(), "js"))
        return {Application, file_exten};
    return {Unknown, ""};
}