//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

int main () {
    JSONValue root{JSONType::objectValue};
    root["str"] = "Hello world";
    root["int"] = 3000;
    root["double"] = 10001.1023;
    root["bool"] = false;
    root["null"] = nullptr;
    root["array"] = JSONValue(JSONType::arrayValue);
    root["array"][0] = 1;
    root["array"][1] = "stt";
    root["array"][2] = JSONValue(JSONType::objectValue);
    root["array"][2]["t"] = "ttt";
    root["object"] = JSONValue(JSONType::objectValue);
    root["object"]["key"] = "ttt";
    root["object"]["obj"] = JSONValue(JSONType::objectValue);
    root["object"]["obj"]["s"] = "test";
    root["object"]["obj"]["a"] = JSONValue(JSONType::arrayValue);
    root["object"]["obj"]["a"][0] = "ttt";
    root["object"]["zzz"] = true;
    std::cerr << root;
    return 0;
}

