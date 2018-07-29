//
// Created by yuwenyong.vincent on 2018/7/29.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.makeCurrent();

    HTTPClient::create()->fetch("https://www.baidu.com/", [](HTTPResponse response){
        std::cout << response.getCode() << std::endl;
        std::cout << response.getBody() << std::endl;
    }, ARG_validateCert=false, ARG_userAgent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:61.0) Gecko/20100101 Firefox/61.0");
    reactor.run();
    return 0;
}