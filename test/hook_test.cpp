#include "Log.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "IOManager.h"

static xb::Logger::ptr stdout_logger = GET_ROOT_LOGGER();

void test_sleep()
{
    xb::IOManager iom(3);
    iom.addTask([]()
                {
        sleep(4);
        LOG_INFO(stdout_logger, "sleep 2"); });

    iom.addTask([]()
                {
        sleep(5);
        LOG_INFO(stdout_logger, "sleep 3"); });
    LOG_INFO(stdout_logger, "test_sleep");
}

void test_sock()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    LOG_INFO(stdout_logger, "begin connect");

    int rt = connect(sock, (const sockaddr *)&addr, sizeof(addr));
    LOG_FMT_INFO(stdout_logger, "connect rt = %d, errno = %d", rt, errno);
    if (rt)
    {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    LOG_FMT_INFO(stdout_logger, "send rt = %d, errno = %d", rt, errno);

    if (rt <= 0)
    {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    LOG_FMT_INFO(stdout_logger, "recv rt = %d, errno = %d", rt, errno);

    if (rt <= 0)
    {
        return;
    }

    buff.resize(rt);
    std::cout << "buff:\n"
              << buff.c_str() << std::endl;
}

int main(int argc, char **argv)
{
    // test_sleep();
    xb::IOManager iom(3);
    iom.addTask([]()
                {
        sleep(6);
        LOG_INFO(stdout_logger, "sleep 6"); });

    iom.addTask([]()
                {
        sleep(4);
        LOG_INFO(stdout_logger, "sleep 4"); });
    // LOG_INFO(stdout_logger, "test_sleep");
    iom.addTask(test_sock);
    return 0;
}
