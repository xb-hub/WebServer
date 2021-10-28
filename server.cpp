//
// Created by 许斌 on 2021/10/27.
//
#define _DEBUG_POOL_
#include "MyHttp/MyHttp.h"
using namespace xb;

int main(int argc, char* argv[])
{
    if(argc < 4)
    {
        std::cout << "Usage: Paramter!" << std::endl;
        return -1;
    }
    MyHttp* http;
    if(!strcasecmp(argv[3], "pool"))
    {
#ifdef _DEBUG_POOL_
        std::cout << "using ThreadPool!" << std::endl;
#endif
        http = new MyHttp(atoi(argv[1]), argv[2], true, atoi(argv[4]));
    }
    else
    {
#ifdef _DEBUG_POOL_
        std::cout << "don't use ThreadPool!" << std::endl;
#endif
        http = new MyHttp(atoi(argv[1]), argv[2], false, 0);
    }
    http->start_up();
    return 0;
}
