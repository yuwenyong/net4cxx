//
// Created by yuwenyong.vincent on 2018/8/25.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

std::map<int, std::string> gBookNames;

class Books: public RequestHandler {
public:
    using RequestHandler::RequestHandler;

    void onGet(const StringVector &args) override {
        JsonValue response;
        response["books"] = JsonType::arrayValue;
        for (auto book: gBookNames) {
            JsonValue b;
            b["id"] = book.first;
            b["name"] = book.second;
            response["books"].append(b);
        }
        write(response);
    }

    void onPost(const StringVector &args) override {
        auto body = boost::lexical_cast<JsonValue>(getRequest()->getBody());
        gBookNames[body["id"].asInt()] = body["name"].asString();
        write(body);
    }
};


class Book: public RequestHandler {
public:
    using RequestHandler::RequestHandler;

    void onGet(const StringVector &args) override {
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
    }

    void onDelete(const StringVector &args) override {
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
    }

    void onPut(const StringVector &args) override {
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
    }
};


int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.makeCurrent();

    std::make_shared<WebApp>(WebApp::HandlersType{
       url<Books>(R"(/books/)"),
       url<Book>(R"(/books/(\d+)/)")
    })->listen(8080, "localhost");
    reactor.run();
    return 0;
}