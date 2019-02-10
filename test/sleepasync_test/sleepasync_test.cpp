//
// Created by yuwenyong.vincent on 2019-01-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class SleepAsyncTest: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        auto d = sleepAsync(reactor(), 2.0f);
        d->addCallback([](DeferredValue result){
            NET4CXX_LOG_INFO("First callback");
            return result;
        })->addErrback([](DeferredValue result){
            NET4CXX_LOG_ERROR("Error happend");
            try {
                result.throwError();
            } catch (std::exception &e) {
                NET4CXX_LOG_ERROR(e.what());
            }
            return nullptr;
        })->addCallback([this](DeferredValue result){
            NET4CXX_LOG_INFO("two seconds later I will trigger later callback");
            return sleepAsync(reactor(), 2.0f);
        })->addCallback([](DeferredValue result) {
            NET4CXX_LOG_INFO("Yeah triggered");
            return result;
        });
        auto d2 = makeDeferred();
        d2->addCallback([](DeferredValue result) {
            NET4CXX_LOG_INFO("chained");
            return nullptr;
        });
        d->chainDeferred(d2);
        d->addTimeout(5.0, reactor(), [](DeferredValue result){
            NET4CXX_LOG_INFO("Timeout");
            return nullptr;
        });
    }
};


int main(int argc, char **argv) {
    SleepAsyncTest app;
    app.run(argc, argv);
    return 0;
}