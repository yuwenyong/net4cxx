//
// Created by yuwenyong.vincent on 2019-01-13.
//
#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class JsonTest: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        JsonValue root{JsonType::objectValue};
        root["str"] = "Hello world";
        root["int"] = 3000;
        root["double"] = 10001.1023;
        root["bool"] = false;
        root["null"] = nullptr;
        root["array"] = JsonValue(JsonType::arrayValue);
        root["array"][0] = 1;
        root["array"][1] = "stt";
        root["array"][2] = JsonValue(JsonType::objectValue);
        root["array"][2]["t"] = "ttt";
        root["object"] = JsonValue(JsonType::objectValue);
        root["object"]["key"] = "ttt";
        root["object"]["obj"] = JsonValue(JsonType::objectValue);
        root["object"]["obj"]["s"] = "test";
        root["object"]["obj"]["a"] = JsonValue(JsonType::arrayValue);
        root["object"]["obj"]["a"][0] = "ttt";
        root["object"]["zzz"] = true;
        JsonValue cp, nu;
        std::cerr << root << std::endl;
        cp = root;
        std::cerr << cp << std::endl;
        if (cp == root) {
            std::cerr << "cp == root" << std::endl;
        }  else {
            std::cerr << "cp != root" << std::endl;
        }
        if (nu == root) {
            std::cerr << "nu == root" << std::endl;
        }  else {
            std::cerr << "nu != root" << std::endl;
        }
        std::stringstream ss;
        JsonValue result;
        ss << cp;
        ss >> result;
        std::cerr << "result" << std::endl;
        std::cerr << result << std::endl;
    }
};


int main(int argc, char **argv) {
    JsonTest app{false};
    app.run(argc, argv);
    return 0;
}