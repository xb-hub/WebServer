#include "Socket.h"
#include "Log.h"
#include <vector>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

namespace xb
{
    Address::Address()
        : addrIn_({0})
    {}

    Address::Address(uint16_t port)
        : isVaild_(true)
    {
        toAddrIpv4Any(port);
    }

    Address::Address(struct sockaddr_in addr)
        : addrIn_(addr),
        isVaild_(true)
    {}

    Address::Address(const std::string& addr,uint16_t port)
    {
        isVaild_ = toAddrIpv4(addr, port);
    }

    Address::~Address()
    {}

    bool Address::toAddrIpv4(const std::string& addrStr)
    {
        auto iter = addrStr.find(':');
        if(iter != addrStr.npos)
        {
            std::string ip = addrStr.substr(0, iter);
            uint16_t port = atoi(addrStr.substr(iter + 1).c_str());
            return toAddrIpv4(ip, port);
        }
        return false;
    }

    bool Address::toAddrIpv4(const std::string& ip, uint16_t port)
    {
        std::vector<int> ip_int;
        size_t first = 0;
        while(true)
        {
            auto last = ip.find('.',first);
            if(last == ip.npos)
            {
                ip_int.push_back(atoi(ip.substr(first,ip.size() - first).c_str()));
                break;
            }
            ip_int.push_back(atoi(ip.substr(first, last- first).c_str()));
            first = last+1;
        }
        if(ip.size()!=4)
        {
            return false;
        }
        uint32_t addr32 ;
        for(int i=0;i<4;i++)
        {
            addr32 <<= 8;
            addr32 |= ip_int[i];
        }
        bzero(&addrIn_, sizeof(addrIn_));
        addrIn_.sin_family = AF_INET;
        addrIn_.sin_port = htons(port);
        addrIn_.sin_addr.s_addr = addr32;
        return true;
    }

    bool Address::toAddrIpv4Any(uint16_t port)
    {
        bzero(&addrIn_, sizeof(addrIn_));
        addrIn_.sin_family = AF_INET;
        addrIn_.sin_port = htons(port);
        addrIn_.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    std::ostream& operator<<(std::ostream& os, Address& addr)
    {
        os << addr.addrIn_.sin_addr.s_addr << ":" << addr.addrIn_.sin_port;
        return os;
    }
    
    Socket::Socket(int family, int type, int protocol, bool openLinger_)
        : sock_fd_(socket(family, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol))
    {
        struct linger optLinger = {0};
        if (openLinger_)
        {
            /* 优雅关闭: 直到所剩数据发送完毕或超时 */
            optLinger.l_onoff = 1;
            optLinger.l_linger = 1;
        }
        setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optLinger, sizeof(optLinger));
    }

    Socket::~Socket()
    {}

    Socket::ptr Socket::createTcpSocket()
    {
        Socket::ptr sock = std::make_shared<Socket>(IPv4, TCP, 0);
        return sock;
    }

    Socket::ptr Socket::createUdpSocket()
    {
        Socket::ptr sock = std::make_shared<Socket>(IPv4, UDP, 0);
        return sock;
    }

    int Socket::Bind(Address& addr)
    {
        int rt = bind(sock_fd_, (struct sockaddr *)addr.getAddrPtr(), sizeof(addr.getAddr()));
        if(rt < 0)
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "Bind Failure!");
        }
        return rt;
    }

    int Socket::Accept(Address& addr)
    {
        struct sockaddr_in  clientAddr{0};
        socklen_t len = sizeof(clientAddr);
        int rt = accept(sock_fd_, (struct sockaddr *)addr.getAddrPtr(), &len);
        if(rt < 0)
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "Accept Failure!");
        }
        else
        {
            addr.setAddr(clientAddr);
        }
        return rt;
    }

    int Socket::Listen()
    {
        int rt = listen(sock_fd_, 10);
        if(rt < 0)
        {
            LOG_DEBUG(GET_ROOT_LOGGER(), "Listen Failure!");
        }
        return rt;
    }

} // namespace xb
