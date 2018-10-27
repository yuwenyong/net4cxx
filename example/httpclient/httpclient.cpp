//
// Created by yuwenyong.vincent on 2018/7/29.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class HTTPClientApp: public AppBootstrapper {
public:
    void onRun() {
        auto request = HTTPRequest::create("https://www.baidu.com/")
                ->setValidateCert(false)
                ->setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:61.0) Gecko/20100101 Firefox/61.0");
        HTTPClient::create()->fetch(request, [](const HTTPResponse &response){
            std::cout << response.getCode() << std::endl;
            std::cout << response.getBody() << std::endl;
        })->addCallback([](DeferredValue value) {
            std::cout << "Success" << std::endl;
            return value;
        });
    }
};

int main(int argc, char **argv) {
    HTTPClientApp app;
    app.run(argc, argv);
    return 0;
}