//
// Created by yuwenyong.vincent on 2018/8/25.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

std::map<int, std::string> gBookNames;

class Books: public RequestHandler {
public:
    using RequestHandler::RequestHandler;

    DeferredPtr prepare() override {
        return testAsyncFunc();
    }

    DeferredPtr onGet(const StringVector &args) override {
        return testAsyncFunc2();
    }

    DeferredPtr onPost(const StringVector &args) override {
        auto body = boost::lexical_cast<JsonValue>(getRequest()->getBody());
        gBookNames[body["id"].asInt()] = body["name"].asString();
        write(body);
        return nullptr;
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
            for (auto book: gBookNames) {
                JsonValue b;
                b["id"] = book.first;
                b["name"] = book.second;
                response["books"].append(b);
            }
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


class Book: public RequestHandler {
public:
    using RequestHandler::RequestHandler;


    DeferredPtr onGet(const StringVector &args) override {
        auto bookId = std::stoi(args[0]);
        auto iter = gBookNames.find(bookId);
        if (iter != gBookNames.end()) {
            JsonValue response;
            response["book"]["id"] = iter->first;
            response["book"]["name"] = iter->second;
            write(response);
        } else {
            sendError(404);
        }
        return nullptr;
    }

    DeferredPtr onDelete(const StringVector &args) override {
        auto bookId = std::stoi(args[0]);
        auto iter = gBookNames.find(bookId);
        if (iter != gBookNames.end()) {
            JsonValue response;
            response["book"]["id"] = iter->first;
            response["book"]["name"] = iter->second;
            gBookNames.erase(iter);
            write(response);
        } else {
            sendError(404);
        }
        return nullptr;
    }

    DeferredPtr onPut(const StringVector &args) override {
        auto bookId = std::stoi(args[0]);
        auto iter = gBookNames.find(bookId);
        if (iter != gBookNames.end()) {
            JsonValue response;
            iter->second = getArgument("name");
            response["book"]["id"] = iter->first;
            response["book"]["name"] = iter->second;
            write(response);
        } else {
            sendError(404);
        }
        return nullptr;
    }
};

class HTTPServerApp: public AppBootstrapper {
public:
    void onRun() {
        auto webApp = makeWebApp<WebApp>({
                                                 url<Books>(R"(/books/)"),
                                                 url<Book>(R"(/books/(\d+)/)")
                                         });
        reactor()->listenTCP("8080", std::move(webApp));
    }
};


int main(int argc, char **argv) {
    HTTPServerApp app;
    app.run(argc, argv);
    return 0;
}