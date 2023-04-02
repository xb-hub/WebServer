//
// Created by 许斌 on 2021/10/27.
//
// #define _DEBUG_
// #define _DEBUG_MESSAGE_
// #define _DEBUG_TYPE_
#define _DEBUG_HEAD_

#include "Server.h"

namespace xb
{

    // static Logger::ptr logger = GET_ROOT_LOGGER();

    int Server::setnonblocking(int fd)
    {
        return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    }

    // 构造函数初始化
    Server::Server(const int port, const std::string &resources)
        : listenfd_(-1),
          root_(resources), // "/Users/xubin/Desktop/MyHttp/htdocs"
          port_(port),
          timeoutMs(60000),
          isClose_(false),
          openLinger_(false),
          epoller_(new Epoller())
    {
        HttpConn::userCount = 0;
        HttpConn::srcDir = root_.c_str();
        InitEventMode(3);
        InitSocket();
        SqlConnPool::Instance()->Init("localhost", 3306, "root", "123456", "webserver", 12);
        // pool = std::make_shared<ThreadPool>();
        // pool->start();
        schedule = std::make_unique<Scheduler>();
        schedule->start();
        timer = std::make_shared<TimerManager>();
    }

    // 析构函数
    Server::~Server()
    {
        // if(pool)    pool->stop();
        close(listenfd_);
    }

    void Server::InitEventMode(int trigMode)
    {
        listenEvent_ = EPOLLRDHUP;
        connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
        switch (trigMode)
        {
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            connEvent_ |= EPOLLET;
            listenEvent_ |= EPOLLET;
            break;
        default:
            connEvent_ |= EPOLLET;
            listenEvent_ |= EPOLLET;
            break;
        }
        HttpConn::isET = (connEvent_ & EPOLLET);
    }

    void Server::AddClient(int fd)
    {
        struct sockaddr_in client_addr{0};
        socklen_t len = sizeof(client_addr);
        do
        {
            int clientfd = accept(fd, (struct sockaddr *)(&client_addr), &len);
            if (clientfd <= 0)
            {
                return;
            }
            else if (HttpConn::userCount >= MAX_FD)
            {
                SendError(fd, "Server busy!");
                LOG_WARN(GET_ROOT_LOGGER(), "Clients is full!");
                return;
            }
            // LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "Add Client : [%d]", clientfd);
            users_[clientfd].init(clientfd, client_addr);
            if(!timer_list_.count(clientfd))
                timer_list_[clientfd] = timer->addTimer(timeoutMs, std::bind(&Server::CloseConn, this, &users_[clientfd]), false);
            epoller_->AddFd(clientfd, EPOLLIN | connEvent_);
            setnonblocking(clientfd);
        } while (listenEvent_ & EPOLLET);
    }

    void Server::ReadRequest(HttpConn *client)
    {
        timer_list_[client->GetFd()]->reset(timeoutMs, false);
        assert(client);
        int ret = -1;
        int readErrno = 0;
        ret = client->read(&readErrno);
        // LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "Read Request : [%d]  [%d]  [%d]", client->GetFd(), ret, readErrno);
        if (ret <= 0 && readErrno != EAGAIN)
        {
            LOG_FMT_INFO(GET_ROOT_LOGGER(), "Client[%d] quit!", client->GetFd());
            CloseConn(client);
            return;
        }
        OnProcess(client);
    }

    void Server::SendReponse(HttpConn *client)
    {
        timer_list_[client->GetFd()]->reset(timeoutMs, false);
        // LOG_DEBUG(GET_ROOT_LOGGER(), "Send Reponse!");
        assert(client);
        int ret = -1;
        int writeErrno = 0;
        ret = client->write(&writeErrno);
        // LOG_FMT_INFO(GET_ROOT_LOGGER(), "Client ERROR!  [%d]", client->ToWriteBytes());
        if (client->ToWriteBytes() == 0)
        {
            /* 传输完成 */
            // LOG_FMT_INFO(GET_ROOT_LOGGER(), " 传输完成 %d ", client->IsKeepAlive());
            if (client->IsKeepAlive())
            {
                OnProcess(client);
                return;
            }
        }
        else if (ret < 0)
        {
            if (writeErrno == EAGAIN)
            {
                /* 继续传输 */
                // LOG_FMT_INFO(GET_ROOT_LOGGER(), " 继续传输 %d ", client->GetFd());
                // pool->AddTask(std::bind(&Server::SendReponse, this, client));
                epoller_->ModFd(client->GetFd(), EPOLLOUT | connEvent_);
                return;
            }
        }
        // LOG_FMT_INFO(GET_ROOT_LOGGER(), "Client[%d   %d] quit!", client->GetFd(), ret);
        CloseConn(client);
    }

    void Server::DealRead(HttpConn* client)
    {
        timer_list_[client->GetFd()]->reset(timeoutMs, true);
        // pool->AddTask(std::bind(&Server::ReadRequest, this, client));
        schedule->addTask(std::bind(&Server::ReadRequest, this, client));
    }

    void Server::DealWrite(HttpConn* client)
    {
        timer_list_[client->GetFd()]->reset(timeoutMs, true);
        // pool->AddTask(std::bind(&Server::SendReponse, this, client));
        schedule->addTask(std::bind(&Server::SendReponse, this, client));
    }

    void Server::OnProcess(HttpConn *client)
    {
        if (client->process())
        {
            //            LOG_FMT_INFO(GET_ROOT_LOGGER(), "Process WRITE : [%d]", client->GetFd());
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
        }
        else
        {
            //            LOG_FMT_INFO(GET_ROOT_LOGGER(), "Process READ : [%d]", client->GetFd());
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
        }
    }

    void Server::SendError(int fd, const char *info)
    {
        assert(fd > 0);
        int ret = send(fd, info, strlen(info), 0);
        if (ret < 0)
        {
            LOG_FMT_WARN(GET_ROOT_LOGGER(), "send error to client[%d] error!", fd);
        }
        close(fd);
    }


    void Server::CloseConn(HttpConn *client)
    {
        assert(client->GetFd());
        // LOG_FMT_INFO(GET_ROOT_LOGGER(), "Client[%d] quit!", client->GetFd());
        epoller_->DelFd(client->GetFd());
        client->Close();
        // LOG_FMT_INFO(GET_ROOT_LOGGER(), "Client[%d] quit!", client->GetFd());
    }

    // 启动服务器
    void Server::process()
    {
        std::vector<std::function<void()>> timers;
        while (true)
        {
            uint64_t timeout = timer->getNextTimer();
            // LOG_INFO(GET_ROOT_LOGGER(), "epoll wait");
            int rt = epoller_->Wait(timeout);
            timer->listExpiredCb(timers); // 获取超时任务
            // LOG_FMT_INFO(GET_ROOT_LOGGER(), "epoll wait: [%d]", timers.size());
            for(auto &it : timers)
            {
                // pool->AddTask(it); // 函数添加到调度器中
                schedule->addTask(it);
            }
            timers.clear();
            for(int i = 0; i < rt; i++)
            {
                /* 处理事件 */
                int fd = epoller_->GetEventFd(i);
                // LOG_FMT_INFO(GET_ROOT_LOGGER(), "epoll wait : [%d]", fd);
                uint32_t events = epoller_->GetEvents(i);
                if(fd == listenfd_) {
                    AddClient(listenfd_);
                }
                else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    assert(users_.count(fd) > 0);
                    CloseConn(&users_[fd]);
                }
                else if(events & EPOLLIN) {
                    // LOG_FMT_INFO(GET_ROOT_LOGGER(), "EPOLL READ : [%d]", fd);
                    assert(users_.count(fd) > 0);
                    DealRead(&users_[fd]);
                }
                else if(events & EPOLLOUT) {
                    assert(users_.count(fd) > 0);
                    // LOG_FMT_INFO(GET_ROOT_LOGGER(), "EPOLL WRITE : [%d]", fd);
                    DealWrite(&users_[fd]);
                } else {
                    LOG_ERROR(GET_ROOT_LOGGER(), "Unexpected event");
                }
            }
        }
    }

    void Server::InitSocket()
    {
        struct linger optLinger = {0};
        if (openLinger_)
        {
            /* 优雅关闭: 直到所剩数据发送完毕或超时 */
            optLinger.l_onoff = 1;
            optLinger.l_linger = 1;
        }
        if ((listenfd_ = socket(PF_INET, SOCK_STREAM, 0)) < 0) // 创建套接字
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "socket create failure");
        }
        /**
         * setsockopt选项
         * SO_REUSEADDR     避免TIME_WAIT状态
         * SO_RCVBUF SO_SNDBUF  接收缓冲区和发送缓冲区大小
         * SO_RCVLOWAT SO_SNDLOWAT  接收缓冲区和发送缓冲区低水位标志
         */
        int ret = setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
        if (ret < 0)
        {
            close(listenfd_);
            return;
        }

        int optval = 1;
        /* 端口复用 */
        /* 只有最后一个套接字会正常接收数据。 */
        ret = setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
        if (ret == -1)
        {
            close(listenfd_);
            return;
        }

        struct sockaddr_in addr = {0};
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listenfd_, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "socket bing failure");
        }
        if (listen(listenfd_, 6) < 0)
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "socket listen failure");
        }
        ret = epoller_->AddFd(listenfd_, listenEvent_ | EPOLLIN);
        if (ret == 0)
        {
            LOG_ERROR(GET_ROOT_LOGGER(), "Add listen error!");
            close(listenfd_);
            return;
        }
        setnonblocking(listenfd_);
        LOG_FMT_INFO(GET_ROOT_LOGGER(), "Server port:%d", port_);
        return;
    }

} // namespace xb