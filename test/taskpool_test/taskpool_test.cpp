//
// Created by yuwenyong.vincent on 2019-01-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class TaskPoolTest: public AppBootstrapper {
public:
    using AppBootstrapper::AppBootstrapper;

    void onRun() override {
        TaskPool taskPool;
        taskPool.start(4);
        std::cerr << "Started" << std::endl;
        auto f1 = taskPool.submit([]() {
            std::cerr << "First task" << std::endl;
            std::cerr << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds{5});
            std::cerr << "First task completed" << std::endl;
            return 17;
        });

        auto f2 = taskPool.submit([]() {
            std::cerr << "Second task" << std::endl;
            std::cerr << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds{2});
            std::cerr << "Second task completed" << std::endl;
            return 18.1;
        });

        auto f3 = taskPool.submit([]() {
            std::cerr << "Third task" << std::endl;
            std::cerr << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds{3});
            std::cerr << "Third task completed" << std::endl;
            return std::string{"abc"};
        });

        auto f4 = taskPool.submit([]() {
            std::cerr << "Forth task" << std::endl;
            std::cerr << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds{1});
            std::cerr << "Forth task completed" << std::endl;
            return 4;
        });

        auto f5 = taskPool.submit([]() {
            std::cerr << "Fifth task" << std::endl;
            std::cerr << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds{3});
            std::cerr << "Fifth task completed" << std::endl;
            return 5;
        });

        auto val1 = f1.get();
        auto val2 = f2.get();
        auto val3 = f3.get();
        auto val4 = f4.get();
        auto val5 = f5.get();
        std::cerr << "First result:" << val1 << std::endl;
        std::cerr << "Second result:" << val2 << std::endl;
        std::cerr << "Third result:" << val3 << std::endl;
        std::cerr << "Forth result:" << val4 << std::endl;
        std::cerr << "Fifth result:" << val5 << std::endl;

        taskPool.stop();
        taskPool.wait();
        std::cerr << "Completed" << std::endl;
    }
};


int main(int argc, char **argv) {
    TaskPoolTest app{false};
    app.run(argc, argv);
    return 0;
}