//
// Created by 许斌 on 2021/10/27.
//
#define _DEBUG_POOL_
#include "MyHttp/MyHttp.h"
#include <boost/program_options.hpp>
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
             ("root,r", boost::program_options::value<std::string>(&root)->default_value("/Users/xubin/Desktop/WebServer/htdocs"), "root path")
             ("pool,y", boost::program_options::value<std::string>(&is_using)->default_value("pool"), "using thread pool")
             ("thread,t", boost::program_options::value<int>(&thread_number)->default_value(10), "thread number");
    boost::program_options::positional_options_description pdesc;
    pdesc.add("input", 1);

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).positional(pdesc).run(), vm);
    boost::program_options::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

//
//    boost::program_options::variables_map vm;

    // 创建MyHttp实例
    std::shared_ptr<MyHttp> http;
    if(!strcasecmp(is_using.c_str(), "pool"))
    {
#ifdef _DEBUG_POOL_
        std::cout << "using ThreadPool!" << std::endl;
#endif
        http = std::shared_ptr<MyHttp>(new MyHttp(port, root, true, thread_number));
    }
    else
    {
#ifdef _DEBUG_POOL_
        std::cout << "don't use ThreadPool!" << std::endl;
#endif
        http = std::shared_ptr<MyHttp>(new MyHttp(port, root, false, 0));
    }
    // 启动服务器
    http->start_up();
    return 0;
}
