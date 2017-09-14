//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/net4cxx.h"

void test2();


void test();

class MyCls {
public:
    MyCls() {
        std::cout << "Construct" << std::endl;
    }

    ~MyCls() {
        std::cout << "Destruct" << std::endl;
    }

    void disp() {
        std::cout << "disp" << std::endl;
    }
};

class MyCls2 {
public:
    MyCls2() {
        std::cout << "Construct2" << std::endl;
    }

    ~MyCls2() {
        std::cout << "Destruct2" << std::endl;
    }

    void disp() {
        std::cout << "disp2" << std::endl;
    }
};

int main () {
    net4cxx::Singleton<MyCls>::instance()->disp();
    net4cxx::Singleton<MyCls2>::instance()->disp();
//    try {
//        net4cxx::CrashReport::printCrashInfo();
//        BOOST_ASSERT(false);
//        test();
//    } catch (std::exception &e) {
//        std::cerr << e.what() << std::endl;
//    }
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