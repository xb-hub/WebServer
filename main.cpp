//
// Created by 许斌 on 2021/10/27.
//
#define _DEBUG_POOL_
#include "Server.h"
#include <boost/program_options.hpp>
#include <yaml-cpp/yaml.h>
using namespace xb;

int main(int argc, char* argv[])
{
    int port, thread_number;
    std::string root, is_using;
    // 判断参数个数是否符合要求
//    if(argc < 4)
//    {
//        std::cout << "Usage: Paramter!" << std::endl;
//        return -1;
//    }
    boost::program_options::options_description desc("Allowed options");
         desc.add_options()
             ("help", "produce help message")
             ("port,p", boost::program_options::value<int>(&port)->default_value(4000), "port number")
             ("root,r", boost::program_options::value<std::string>(&root)->default_value("/home/xubin/Desktop/WebServer/resources"), "root path")
             ("thread,t", boost::program_options::value<int>(&thread_number)->default_value(10), "thread number");
    boost::program_options::positional_options_description pdesc;
    pdesc.add("port", 1).add("root", 1).add("thread", 1);

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).positional(pdesc).run(), vm);
    boost::program_options::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }
    // 创建MyHttp实例
    std::shared_ptr<Server> http = std::make_shared<Server>(port, root);
    http->process();
    return 0;
}
