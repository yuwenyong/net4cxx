//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

NET4CXX_DECLARE_EXCEPTION(Test1Error, Exception);
NET4CXX_DECLARE_EXCEPTION(Test2Error, Test1Error);

int test1(int arg) {
    NET4CXX_THROW_EXCEPTION(Test2Error, "Hehe");
    return 0;
}

int test2(int arg1, int arg2) {
    return test1(arg1);
}

int test3(int arg1, int arg2) {
    return test2(arg1, arg2);
}

int main () {

    try {
        test3(0, 0);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

//int main() {
//    JSONValue root{JSONType::objectValue};
//    root["str"] = "Hello world";
//    root["int"] = 3000;
//    root["double"] = 10001.1023;
//    root["bool"] = false;
//    root["null"] = nullptr;
//    root["array"] = JSONValue(JSONType::arrayValue);
//    root["array"][0] = 1;
//    root["array"][1] = "stt";
//    root["array"][2] = JSONValue(JSONType::objectValue);
//    root["array"][2]["t"] = "ttt";
//    root["object"] = JSONValue(JSONType::objectValue);
//    root["object"]["key"] = "ttt";
//    root["object"]["obj"] = JSONValue(JSONType::objectValue);
//    root["object"]["obj"]["s"] = "test";
//    root["object"]["obj"]["a"] = JSONValue(JSONType::arrayValue);
//    root["object"]["obj"]["a"][0] = "ttt";
//    root["object"]["zzz"] = true;
//    JSONValue cp, nu;
//    std::cerr << root << std::endl;
//    cp = root;
//    std::cerr << cp << std::endl;
//    if (cp == root) {
//        std::cerr << "cp == root" << std::endl;
//    }  else {
//        std::cerr << "cp != root" << std::endl;
//    }
//    if (nu == root) {
//        std::cerr << "nu == root" << std::endl;
//    }  else {
//        std::cerr << "nu != root" << std::endl;
//    }
//    std::stringstream ss;
//    JSONValue result;
//    ss << cp;
//    ss >> result;
//    std::cerr << "result" << std::endl;
//    std::cerr << result << std::endl;
//}
