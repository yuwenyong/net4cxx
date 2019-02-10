//
// Created by yuwenyong.vincent on 2019-01-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class Books: public RequestHandler {
public:
    using RequestHandler::RequestHandler;

    DeferredPtr prepare() override {
        return testAsyncFunc();
    }

    DeferredPtr onGet(const StringVector &args) override {
        return testAsyncFunc2();
    }

    DeferredPtr testAsyncFunc() {
        auto request = HTTPRequest::create("https://www.baidu.com/")
                ->setValidateCert(false)
                ->setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:61.0) Gecko/20100101 Firefox/61.0");
        return HTTPClient::create()->fetch(request, [this, self=shared_from_this()](const HTTPResponse &response){
            std::cout << response.getCode() << std::endl;
            getArgument("name");
        })->addCallbacks([](DeferredValue value) {
            std::cout << "Success" << std::endl;
            return value;
        }, [](DeferredValue value) {
            std::cout << "Fail" << std::endl;
            return value;
        });
    }

    DeferredPtr testAsyncFunc2() {
        auto request = HTTPRequest::create("https://www.baidu.com/")
                ->setValidateCert(false)
                ->setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:61.0) Gecko/20100101 Firefox/61.0");
        return HTTPClient::create()->fetch(request, [this, self=shared_from_this()](const HTTPResponse &resp){
            std::cout << resp.getCode() << std::endl;
//            getArgument("name");
            JsonValue response;
            response["books"] = JsonType::arrayValue;
            write(response);
        })->addCallbacks([](DeferredValue value) {
            std::cout << "Success" << std::endl;
            return value;
        }, [](DeferredValue value) {
            std::cout << "Fail" << std::endl;
            return value;
        });
    }
};


class HTTPServerAsyncTest: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        auto webApp = makeWebApp<WebApp>({
                                                 url<Books>(R"(/books/)"),
                                         });
        reactor()->listenTCP("8080", std::move(webApp));
    }
};


int main(int argc, char **argv) {
    HTTPServerAsyncTest app;
    app.run(argc, argv);
    return 0;
}