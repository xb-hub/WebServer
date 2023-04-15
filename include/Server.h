#ifndef _SERVER_H_
#define _SERVER_H_
#include <iostream>
#include <fstream>
#include <string.h>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <unordered_map>
#include <fcntl.h>
#include <thread>
#include "Timer.h"
#include "ThreadPool.h"
#include "Scheduler.h"
#include "IOEventLoop.h"
#include "HttpConnection.h"
#include "Log.h"
#include "util.h"
#include "Socket.h"

namespace xb
{

    class Server
    {

    public:
        Server(const int port, const std::string &resources);
        ~Server();

    private:
        std::string root_; // 服务器文件根目录
        bool isClose_;
        bool openLinger_;
        uint64_t timeoutMs;

        uint32_t listenEvent_;
        uint32_t connEvent_;

        ThreadPool::ptr pool_;
        Scheduler::unptr schedule_;
        TimerManager::ptr timer_;
        IOEventLoop::ptr loop_;
        Socket::ptr socket_;
        Address address_;

    public:
        void process();

        static int setnonblocking(int fd);

        IOEventLoop* getLoop() const { return loop_.get(); }

        // bool Server::stopping(uint64_t &timeout);

    private:
        void InitSocket();
        void InitEventMode(int trigMode);

        void ReadRequest(HttpConnection* client);
        void SendReponse(HttpConnection* client);
        void AddClient(int fd);

        void SendError(int fd, const char*info);
        void CloseConn(HttpConnection *client);

        void OnProcess(HttpConnection* client);

        void LoopAddEvent(HttpConnection *client, bool is_read);
        void LoopModEvent(HttpConnection *client, bool is_read);
        void LoopDelEvent(HttpConnection *client, bool is_read);

        static const int MAX_FD = 65536;

        std::unordered_map<int, HttpConnection> users_;
        std::unordered_map<int, Timer::ptr> timer_list_;
    };

} // namespace xb

#endif