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
    Server::Server(int port, const std::string &resources)
        : address_(port),
          root_(resources), // "/Users/xubin/Desktop/MyHttp/htdocs"
          timeoutMs(60000),
          isClose_(false),
          openLinger_(false),
          pool_(std::make_shared<ThreadPool>()),
          loop_(std::make_shared<IOEventLoop>())
    {
        HttpConnection::userCount = 0;
        HttpConnection::srcDir = root_.c_str();
        InitEventMode(3);
        socket_ = Socket::createTcpSocket();
        SqlConnPool::Instance()->Init("localhost", 3306, "root", "123456", "webserver", 12);
        LOG_DEBUG(GET_ROOT_LOGGER(), "创建数据库");
        pool_->start();
        LOG_DEBUG(GET_ROOT_LOGGER(), "创建线程池");
        // schedule_ = std::make_unique<Scheduler>();
        // schedule_->start();
        timer_ = std::make_shared<TimerManager>();
    }

    // 析构函数
    Server::~Server()
    {
        if(pool_)    pool_->stop();
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
        HttpConnection::isET = (connEvent_ & EPOLLET);
    }

    void Server::AddClient(int fd)
    {
        // LOG_DEBUG(GET_ROOT_LOGGER(), "Add Client!");
        struct sockaddr_in client_addr{0};
        socklen_t len = sizeof(client_addr);
        do
        {
            int clientfd = accept(fd, (struct sockaddr *)(&client_addr), &len);
            if (clientfd <= 0)
            {
                return;
            }
            else if (HttpConnection::userCount >= MAX_FD)
            {
                SendError(fd, "Server busy!");
                LOG_WARN(GET_ROOT_LOGGER(), "Clients is full!");
                return;
            }
            // LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "Add Client : [%d]", clientfd);
            users_[clientfd].init(clientfd, client_addr);
            if(!timer_list_.count(clientfd))
                timer_list_[clientfd] = timer_->addTimer(timeoutMs, std::bind(&Server::CloseConn, this, &users_[clientfd]), false);
            // epoller_->AddFd(clientfd, EPOLLIN | connEvent_);
            setnonblocking(clientfd);
            IOEventLoop::ptr loop = pool_->getOneLoopFromPool();
            users_[clientfd].setLoop(loop);
            LoopAddEvent(&users_[clientfd], true);
            
        } while (listenEvent_ & EPOLLET);
    }

    void Server::ReadRequest(HttpConnection *client)
    {
        timer_list_[client->GetFd()]->reset(timeoutMs, false);
        assert(client);
        size_t ret = -1;
        int readErrno = 0;
        ret = client->read(&readErrno);
        // LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "Read Request : [%d]  [%d]  [%d]", client->getLoop()->getEpollFd(), client->GetFd(), ret);
        if (ret <= 0 && readErrno != EAGAIN && readErrno != EWOULDBLOCK)
        {
            // LOG_FMT_INFO(GET_ROOT_LOGGER(), "ReadRequest[%d] quit!", client->GetFd());
            CloseConn(client);
            return;
        }
        OnProcess(client);
    }

    void Server::SendReponse(HttpConnection *client)
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
            if (writeErrno == EAGAIN || writeErrno == EWOULDBLOCK)
            {
                /* 继续传输 */
                //  LOG_FMT_INFO(GET_ROOT_LOGGER(), " 继续传输 %d ", client->GetFd());
                // pool->AddTask(std::bind(&Server::SendReponse, this, client));
                // epoller_->ModFd(client->GetFd(), EPOLLOUT | connEvent_);
                LoopModEvent(client, false);
                return;
            }
        }
//        LOG_FMT_INFO(GET_ROOT_LOGGER(), "SendReponse[%d   %d] quit!", client->GetFd(), ret);
        CloseConn(client);
    }

    void Server::OnProcess(HttpConnection *client)
    {
        if (client->process())
        {
            // LOG_FMT_INFO(GET_ROOT_LOGGER(), "Process WRITE : [%d]", client->GetFd());
            // epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            LoopModEvent(client, false);
        }
        else
        {
            // LOG_FMT_INFO(GET_ROOT_LOGGER(), "Process READ : [%d]", client->GetFd());
            LoopModEvent(client, true);
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


    void Server::CloseConn(HttpConnection *client)
    {
        assert(client->GetFd());
        // LOG_FMT_INFO(GET_ROOT_LOGGER(), "Client[%d] quit!", client->GetFd());
        
        client->Close();
//        LOG_FMT_INFO(GET_ROOT_LOGGER(), "Client[%d] quit!", client->GetFd());
    }

    void Server::LoopAddEvent(HttpConnection *client, bool is_read)
    {
        IOEventLoop::ptr loop = client->getLoop();
        int fd = client->GetFd();
        IOEvent::ptr event = loop->getFdEvent(fd);
        if(is_read) 
        {
            // LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "Add Event [%d] READ To [%d]", fd, loop->getEpollFd());
            event->enableReading();
            event->setReadEventHandler(std::bind(&Server::ReadRequest, this, client));
        }
        else
        {
            // LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "Add Event [%d] WRITE To [%d]", fd, loop->getEpollFd());
            event->enableWriting();
            event->setWriteEventHandler(std::bind(&Server::SendReponse, this, client));
        }
        
        loop->AddEvent(event);
    }

    void Server::LoopModEvent(HttpConnection *client, bool is_read)
    {
        IOEventLoop::ptr loop = client->getLoop();
        int fd = client->GetFd();
        IOEvent::ptr event = std::make_shared<IOEvent>(fd);
        if(is_read)
        {
            // LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "Add Event [%d] READ To [%d]", fd, loop->getEpollFd());
            event->enableReading();
            event->setReadEventHandler(std::bind(&Server::ReadRequest, this, client));
        }
        else
        {
            // LOG_FMT_DEBUG(GET_ROOT_LOGGER(), "Add Event [%d] WRITE To [%d]", fd, loop->getEpollFd());
            event->enableWriting();
            event->setWriteEventHandler(std::bind(&Server::SendReponse, this, client));
        }

        loop->ModEvent(event);
    }

    // 启动服务器
    void Server::process()
    {
        InitSocket();
        // io_manager_->addEvent(listenfd_, FdEventType::READ, std::bind(&Server::AddClient, this, listenfd_));
        // std::vector<std::function<void()>> timers;
        // while (true)
        // {
        //     uint64_t timeout = timer->getNextTimer();
        //     // LOG_INFO(GET_ROOT_LOGGER(), "epoll wait");
        //     int rt = epoller_->Wait(timeout);
        //     timer->listExpiredCb(timers); // 获取超时任务
        //     // LOG_FMT_INFO(GET_ROOT_LOGGER(), "epoll wait: [%d]", timers.size());
        //     for(auto &it : timers)
        //     {
        //         // pool->AddTask(it); // 函数添加到调度器中
        //         schedule->addTask(it);
        //     }
        //     timers.clear();
        //     for(int i = 0; i < rt; i++)
        //     {
        //         /* 处理事件 */
        //         int fd = epoller_->GetEventFd(i);
        //         // LOG_FMT_INFO(GET_ROOT_LOGGER(), "epoll wait : [%d]", fd);
        //         uint32_t events = epoller_->GetEvents(i);
        //         if(fd == listenfd_) {
        //             AddClient(listenfd_);
        //         }
        //         else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        //             assert(users_.count(fd) > 0);
        //             CloseConn(&users_[fd]);
        //         }
        //         else if(events & EPOLLIN) {
        //             // LOG_FMT_INFO(GET_ROOT_LOGGER(), "EPOLL READ : [%d]", fd);
        //             assert(users_.count(fd) > 0);
        //             DealRead(&users_[fd]);
        //         }
        //         else if(events & EPOLLOUT) {
        //             assert(users_.count(fd) > 0);
        //             // LOG_FMT_INFO(GET_ROOT_LOGGER(), "EPOLL WRITE : [%d]", fd);
        //             DealWrite(&users_[fd]);
        //         } else {
        //             LOG_ERROR(GET_ROOT_LOGGER(), "Unexpected event");
        //         }
        //     }
        // }
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
        int listenfd = socket_->getSocketFd();
        int ret = setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
        int optval = 1;
        /* 端口复用 */
        /* 只有最后一个套接字会正常接收数据。 */
        ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
        if(ret == -1) {
            LOG_ERROR(GET_ROOT_LOGGER(), "set socket setsockopt error !");
            close(listenfd);
            return;
        }

        socket_->Bind(address_);

        socket_->Listen();

        setnonblocking(listenfd);

        IOEvent::ptr event = std::make_shared<IOEvent>(socket_->getSocketFd());
        event->setReadEventHandler(std::bind(&Server::AddClient, this, socket_->getSocketFd()));
        event->enableReading();
        loop_->AddEvent(event);
    }

} // namespace xb