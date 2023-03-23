#include "Http.h"
#include "Log.h"

void test_request() {
    xb::http::HttpRequest::ptr req(new xb::http::HttpRequest);
    req->setHeader("host" , "www.baidu.con");
    req->setBody("hello");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    xb::http::HttpResponse::ptr rsp(new xb::http::HttpResponse);
    rsp->setHeader("X-X", "xb");
    rsp->setBody("hello");
    rsp->setStatus((xb::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
