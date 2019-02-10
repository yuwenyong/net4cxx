//
// Created by yuwenyong.vincent on 2019-01-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class PeriodCallbackTest: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        PeriodicCallback::create([](){
            NET4CXX_LOG_INFO("Every two seconds");
        }, 2.0f)->start();
    }
};


int main(int argc, char **argv) {
    PeriodCallbackTest app;
    app.run(argc, argv);
    return 0;
}