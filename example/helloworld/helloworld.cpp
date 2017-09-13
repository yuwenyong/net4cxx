//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/net4cxx.h"

void test2();


void test();

int main () {
    try {
        net4cxx::CrashReport::printCrashInfo();
        BOOST_ASSERT(false);
        test();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

void test2() {
    std::cout << boost::stacktrace::stacktrace() << std::endl;
    NET4CXX_ASSERT(false, "addw %s", "dd");
    NET4CXX_THROW_EXCEPTION(net4cxx::Exception, "wocaonima");
}

void test() {
    test2();
}