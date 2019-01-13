//
// Created by yuwenyong.vincent on 2019-01-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class DeferredTest: public CommonBootstrapper {
public:
    void onRun() override {
        auto d1 = makeDeferred();
        d1->addCallback([](DeferredValue value) {
            std::cout << "success:" << *value.asValue<std::string>() << std::endl;
            return std::string{"yyy"};
        })->addErrback([](DeferredValue value) {
            std::cout << "error:" << value.isError() << std::endl;
            return nullptr;
        })->addCallback([](DeferredValue value) {
            std::cout << "success:" << *value.asValue<std::string>() << std::endl;
            return 2;
        });
        d1->callback(std::string{"xxx"});

        auto d2 = makeDeferred();
        d2->addBoth([](DeferredValue value){
            std::cout << "callback isError:" << value.isError() << std::endl;
            try {
                value.throwError();
            } catch (std::exception &e) {
                std::cout << e.what() << std::endl;
            }
            return value;
        });
        d2->errback(std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ValueError, "value error")));
    }
};


int main(int argc, char **argv) {
    DeferredTest app;
    app.run(argc, argv);
    return 0;
}