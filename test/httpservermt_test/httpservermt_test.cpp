//
// Created by yuwenyong.vincent on 2019-01-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class Books: public RequestHandler {
public:
    using RequestHandler::RequestHandler;

    DeferredPtr onGet(const StringVector &args) override {
        std::cerr << "ThreadId:" << std::this_thread::get_id() << std::endl;
        JsonValue response;
        response["books"] = JsonType::arrayValue;
        write(response);
        return nullptr;
    }
};


class HTTPServerMTTest: public AppBootstrapper {
public:
    void onInit() override {
        AppBootstrapper::onInit();
        enableReactor(4);
    }

    void onRun() override {
        auto webApp = makeWebApp<WebApp>({
                                                 url<Books>(R"(/books/)")
                                         });
        reactor()->listenTCP("8080", std::move(webApp));
    }
};


int main(int argc, char **argv) {
    HTTPServerMTTest app;
    app.run(argc, argv);
    return 0;
}