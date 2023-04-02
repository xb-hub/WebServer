#ifndef _SERVER_H_
#define _SERVER_H_
#include <iostream>
#include <fstream>
#include <string.h>
#include <cstdlib>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <fcntl.h>
#include <thread>
#include "Timer.h"
#include "ThreadPool.h"
#include "Scheduler.h"
#include "HttpConnection.h"
#include "Log.h"
#include "epoller.h"
#include "util.h"

namespace xb
{

    class Server
    {

    public:
        Server(const int port, const std::string &resources);
        ~Server();

    private:
        std::string root_; // 服务器文件根目录
        int listenfd_;     // 服务器套接字
        int port_;         // 端口
        bool isClose_;
        bool openLinger_;
        uint64_t timeoutMs;

        uint32_t listenEvent_;
        uint32_t connEvent_;

        ThreadPool::ptr pool;
        Scheduler::unptr schedule;
        TimerManager::ptr timer;

    public:
        void process();

        static int setnonblocking(int fd);

        // bool Server::stopping(uint64_t &timeout);

    private:
        void InitSocket();
        void InitEventMode(int trigMode);

        void ReadRequest(HttpConnection* client);
        void SendReponse(HttpConnection* client);
        void AddClient(int fd);

        void DealRead(HttpConnection* client);
        void DealWrite(HttpConnection* client);

        void SendError(int fd, const char*info);
        void CloseConn(HttpConnection *client);

        void OnProcess(HttpConnection* client);

        static const int MAX_FD = 65536;

        std::unique_ptr<Epoller> epoller_;

        std::unordered_map<int, HttpConnection> users_;
        std::unordered_map<int, Timer::ptr> timer_list_;
    };

} // namespace xb

#endif