#include "Address.h"
#include "Log.h"

xb::Logger::ptr g_logger = GET_ROOT_LOGGER();

void test()
{
    std::vector<xb::Address::ptr> addrs;

    LOG_INFO(g_logger, "begin");
    bool v = xb::Address::Lookup(addrs, "www.baidu.com:http");
    // bool v = sylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    // bool v = sylar::Address::Lookup(addrs, "www.sylar.top", AF_INET);
    LOG_INFO(g_logger, "end");
    if (!v)
    {
        LOG_ERROR(g_logger, "lookup fail");
        return;
    }

    for (size_t i = 0; i < addrs.size(); ++i)
    {
        std::cout << i << " - " << addrs[i]->toString() << std::endl;
        ;
    }

    auto addr = xb::Address::LookupAny("localhost:4080");
    if (addr)
    {
        std::cout << *addr << std::endl;
    }
    else
    {
        LOG_ERROR(g_logger, "error");
    }
}

void test_iface()
{
    std::multimap<std::string, std::pair<xb::Address::ptr, uint32_t>> results;

    bool v = xb::Address::GetInterfaceAddresses(results);
    if (!v)
    {
        std::cout << "GetInterfaceAddresses fail" << std::endl;
        ;
        return;
    }

    for (auto &i : results)
    {
        std::cout << i.first << " - " << i.second.first->toString() << " - " << i.second.second << std::endl;
        ;
    }
}

void test_ipv4()
{
    // auto addr = sylar::IPAddress::Create("www.sylar.top");
    auto addr = xb::IPAddress::Create("127.0.0.8");
    if (addr)
    {
        LOG_INFO(g_logger, addr->toString());
    }
}

int main(int argc, char **argv)
{
    // test_ipv4();
    test_iface();
    // test();
    return 0;
}
