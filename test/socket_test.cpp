#include "Socket.h"
#include "IOManager.h"

static xb::Logger::ptr g_looger = GET_ROOT_LOGGER();

void test_socket()
{
    // std::vector<xb::Address::ptr> addrs;
    // xb::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    // xb::IPAddress::ptr addr;
    // for(auto& i : addrs) {
    //     std::cout << i->toString();
    //     addr = std::dynamic_pointer_cast<xb::IPAddress>(i);
    //     if(addr) {
    //         break;
    //     }
    // }
    xb::IPAddress::ptr addr = xb::Address::LookupAnyIPAddress("www.baidu.com");
    if (addr)
    {
        std::cout << "get address: " << addr->toString() << std::endl;
    }
    else
    {
        std::cout << "get address fail" << std::endl;
        return;
    }

    xb::Socket::ptr sock = xb::Socket::CreateTCP(addr);
    addr->setPort(80);
    std::cout << "addr=" << addr->toString() << std::endl;
    if (!sock->connect(addr))
    {
        std::cout << "connect " << addr->toString() << " fail" << std::endl;
        return;
    }
    else
    {
        std::cout << "connect " << addr->toString() << " connected" << std::endl;
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if (rt <= 0)
    {
        std::cout << "send fail rt=" << rt << std::endl;
        return;
    }

    std::string buffs;
    buffs.resize(10240);
    rt = sock->recv(&buffs[0], buffs.size());

    if (rt <= 0)
    {
        std::cout << "recv fail rt=" << rt;
        return;
    }

    // buffs.resize(rt);
    LOG_DEBUG(g_looger, buffs);
}

void test2()
{
    xb::IPAddress::ptr addr = xb::Address::LookupAnyIPAddress("www.baidu.com:80");
    if (addr)
    {
        std::cout << "get address: " << addr->toString() << std::endl;
    }
    else
    {
        std::cout << "get address fail" << std::endl;
        return;
    }

    xb::Socket::ptr sock = xb::Socket::CreateTCP(addr);
    if (!sock->connect(addr))
    {
        std::cout << "connect " << addr->toString() << " fail" << std::endl;
        return;
    }
    else
    {
        std::cout << "connect " << addr->toString() << " connected" << std::endl;
    }

    uint64_t ts = xb::GetCurrentUS();
    for (size_t i = 0; i < 100000000ul; ++i)
    {
        if (int err = sock->getError())
        {
            std::cout << "err=" << err << " errstr=" << strerror(err) << std::endl;
            break;
        }

        // struct tcp_info tcp_info;
        // if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //     std::cout << "err";
        //     break;
        // }
        // if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //     std::cout
        //             << " state=" << (int)tcp_info.tcpi_state;
        //     break;
        // }
        static int batch = 100000;
        if (i && (i % batch) == 0)
        {
            uint64_t ts2 = xb::GetCurrentUS();
            std::cout << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us" << std::endl;
            ts = ts2;
        }
    }
}

int main(int argc, char **argv)
{
    xb::IOManager iom;
    iom.addTask(&test_socket);
    // iom.addTask(&test2);
    return 0;
}
