#include "IOManager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include "Log.h"

static xb::Logger::ptr stdout_logger = GET_LOGGER("stdout");

int sock = 0;

void test_fiber() {
    //sleep(3);

    //close(sock);
    //sylar::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6666);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        LOG_FMT_INFO(stdout_logger, "test_fiber sock=%d", sock);
        // LOG_FMT_INFO(stdout_logger, "add event errno=%d", errno);
        xb::IOManager::GetThis()->addEvent(sock, xb::FdEventType::READ, [](){
            LOG_INFO(stdout_logger, "read callback");
        });
        xb::IOManager::GetThis()->addEvent(sock, xb::WRITE, [](){
            LOG_INFO(stdout_logger, "write callback");
            //close(sock);
            xb::IOManager::GetThis()->cancelEvent(sock, xb::READ);
            close(sock);
        });
    }
    else
    {
        LOG_INFO(stdout_logger, "error");
    }

}

void test1() {
    // std::cout << "EPOLLIN=" << EPOLLIN
    //           << " EPOLLOUT=" << EPOLLOUT << std::endl;
    xb::IOManager iom(2, true);
    iom.addTask(test_fiber);
}

xb::Timer::ptr s_timer;
void test_timer() {
    xb::IOManager iom(2);
    s_timer = iom.addTimer(1000, []{
        static int i = 0;
        LOG_FMT_INFO(stdout_logger, "hello timer i = %d", i);
        if (++i == 3) {
            //s_timer->reset(2000, true);
            s_timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv) {
    // test1();
    test_timer();
    return 0;
}
