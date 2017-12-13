//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

int main () {
    std::cout << JSONValue::valueToString(1.312, true, 6) << std::endl;
    return 0;
}

