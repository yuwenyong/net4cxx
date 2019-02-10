//
// Created by yuwenyong.vincent on 2019-01-13.
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

void testMyError() {
    std::string myname = "double";
    bool boy = true;
    NET4CXX_THROW_EXCEPTION(ConnectionDeny, "hehe %s, %s", myname, TypeCast<std::string>(boy)) << errinfo_http_code(404);
}


class ExceptionTest: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        try {
            test3(1, 2);
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
        }

        try {
            testMyError();
        } catch (ConnectionDeny &e) {
            std::cout << e.what() << std::endl;
            std::cout << "code:" << e.getCode() << std::endl;
            std::cout << "reason:" << e.getReason() << std::endl;
        }
        try {
            int i = 3;
            NET4CXX_ASSERT_THROW(i == 4, "i must be 4");
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
        }
    }
};


int main(int argc, char **argv) {
    ExceptionTest app{false};
    app.run(argc, argv);
    return 0;
}