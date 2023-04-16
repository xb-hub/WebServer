#ifndef _SOCKET_H_
#define _SOCKET_H_
#include <string>
#include <netinet/in.h>
#include <memory>
#include <ostream>

namespace xb
{

    class Address
    {

    public:
        using ptr = std::shared_ptr<Address>;
        Address();
        explicit Address(uint16_t port = 4000);
        Address(struct sockaddr_in addr);
        Address(const std::string& addr, uint16_t port);
        Address(const std::string& addrAndport);
        ~Address();
        
        void setAddr(struct sockaddr_in addr) { addrIn_ = addr; }
        struct sockaddr_in getAddr() const { return addrIn_; }
        struct sockaddr_in* getAddrPtr() { return &addrIn_; }
        bool isValid() const { return isVaild_; }
        std::string toString() const;

    private:
        bool toAddrIpv4(const std::string& addrStr);
        bool toAddrIpv4(const std::string& ip, uint16_t port);
        bool toAddrIpv4Any(uint16_t port);

        friend std::ostream& operator<<(std::ostream& os, Address& addr);

    private:
        bool isVaild_;
        struct sockaddr_in addrIn_;
    };

    class Socket
    {
    public:
        using ptr = std::shared_ptr<Socket>;

        enum Type
        {
            TCP = SOCK_STREAM,
            UDP = SOCK_DGRAM
        };

        enum Family
        {
            IPv4 = AF_INET,
            IPv6 = AF_INET6,
            UNIX = AF_UNIX
        };

    public:
        Socket(int family, int type, int protocol = 0);
        ~Socket();

        static Socket::ptr createTcpSocket();
        static Socket::ptr createUdpSocket();

        int setSocketOpt();

        int getSocketFd() const { return sock_fd_; }

        int Bind(Address& addr);
        int Accept(Address& adrr);
        int Listen();

    private:
        int sock_fd_;
    };
    
} // namespace xb

#endif
