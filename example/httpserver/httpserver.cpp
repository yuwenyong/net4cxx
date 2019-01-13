//
// Created by yuwenyong.vincent on 2018/8/25.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

std::map<int, std::string> gBookNames;

class Books: public RequestHandler {
public:
    using RequestHandler::RequestHandler;

    DeferredPtr onGet(const StringVector &args) override {
        JsonValue response;
        response["books"] = JsonType::arrayValue;
        for (auto book: gBookNames) {
            JsonValue b;
            b["id"] = book.first;
            b["name"] = book.second;
            response["books"].append(b);
        }
        write(response);
        return nullptr;
    }

    DeferredPtr onPost(const StringVector &args) override {
        auto body = boost::lexical_cast<JsonValue>(getRequest()->getBody());
        gBookNames[body["id"].asInt()] = body["name"].asString();
        write(body);
        return nullptr;
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
    void onRun() override {
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