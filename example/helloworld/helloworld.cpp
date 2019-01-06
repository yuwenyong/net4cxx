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

std::map<int, std::unique_ptr<int>> x = PtrMapCreator<int, int>()
        (10, std::make_unique<int>(11))
        (11, std::make_unique<int>(12))();


class Person {
public:
    Person() = default;
    Person(std::string name, int age, std::string gender, double height)
            : _name(std::move(name)), _age(age), _gender(std::move(gender)), _height(height) {

    }

    void display() const {
        std::cout << _name << ":" << _age << ":" << _gender << ":" << _height << std::endl;
    }

    template<typename ArchiveT>
    void serialize(ArchiveT &archive) {
        archive & _name & _age & _gender & _height;
    }
protected:
    std::string _name;
    int _age;
    std::string _gender;
    double _height{0.0};
};


DeferredPtr callLater(Reactor *reactor, double timeout) {
    auto d = makeDeferred();
    auto delayed = reactor->callLater(timeout, [d]() {
        d->callback(nullptr);
    });
    d->setCanceller([delayed](DeferredPtr) mutable {
        if (delayed.active()) {
            delayed.cancel();
        }
    });
    return d;
}

void testMyError() {
    std::string myname = "double";
    bool boy = true;
    NET4CXX_THROW_EXCEPTION(ConnectionDeny, "hehe %s, %s", myname, TypeCast<std::string>(boy)) << errinfo_http_code(404);
}

// int main (int argc, char **argv) {
//    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
//    Reactor reactor;
//    reactor.makeCurrent();
//    PeriodicCallback::create([](){
//        NET4CXX_LOG_INFO("Every two seconds");
//    }, 2.0f)->start();
//    reactor.run();
//    try {
//        testMyError();
//    } catch (ConnectionDeny &e) {
//        std::cout << e.what() << std::endl;
//        std::cout << "code:" << e.getCode() << std::endl;
//        std::cout << "reason:" << e.getReason() << std::endl;
//    }
//    try {
//        int i =3;
//        NET4CXX_ASSERT_THROW(i == 3, "i must be 4");
//    } catch (std::exception &e) {
//        std::cout << e.what() << std::endl;
//    }
//    auto request = HTTPRequest::create("http://baidu.com", ARG_method="POST");
//    std::cout << request->getUrl() << std::endl;
//    std::cout << request->getMethod() << std::endl;
    // TrafficStats stats;
    // std::cout << boost::lexical_cast<JsonValue>(stats) << std::endl;
    // return 0;
// };


class HelloWorldApp: public AppBootstrapper {
public:
    void onRun() override {
        NET4CXX_LOG_INFO(gAppLog, "Started");
        auto d1 = sleepAsync(reactor(), 2.0f);
        auto d2 = sleepAsync(reactor(), 3.0f);
        auto d3 = sleepAsync(reactor(), 4.0f);
        d1->addCallbacks([d2](DeferredValue result) {
            NET4CXX_LOG_INFO(gAppLog, "Trigger 1");
            d2->cancel();
            return result;
        }, [](DeferredValue result) {
            NET4CXX_LOG_INFO(gAppLog, "Error 1");
            return result;
        });
        d2->addCallbacks([](DeferredValue result) {
            NET4CXX_LOG_INFO(gAppLog, "Trigger 2");
            return result;
        }, [](DeferredValue result) {
            NET4CXX_LOG_INFO(gAppLog, "Error 2");
            return result;
        });
        d3->addCallbacks([](DeferredValue result) {
            NET4CXX_LOG_INFO(gAppLog, "Trigger 3");
            return result;
        }, [](DeferredValue result) {
            NET4CXX_LOG_INFO(gAppLog, "Error 3");
            return result;
        });
//        auto d = callLater(reactor(), 2.0f);
//        d->addCallback([](DeferredValue result){
//            NET4CXX_LOG_INFO("First callback");
//            return result;
//        })->addErrback([](DeferredValue result){
//            NET4CXX_LOG_ERROR("Error happend");
//            try {
//                result.throwError();
//            } catch (std::exception &e) {
//                NET4CXX_LOG_ERROR(e.what());
//            }
//            return nullptr;
//        })->addCallback([this](DeferredValue result){
//            NET4CXX_LOG_INFO("two seconds later I will trigger later callback");
//            return callLater(reactor(), 2.0f);
//        })->addCallback([](DeferredValue result) {
//            NET4CXX_LOG_INFO("Yeah triggered");
//            return result;
//        });
//        auto d2 = makeDeferred();
//        d2->addCallback([](DeferredValue result) {
//            NET4CXX_LOG_INFO("chained");
//            return nullptr;
//        });
//        d->chainDeferred(d2);
//        d->addTimeout(1.0, reactor(), [](DeferredValue result){
//            NET4CXX_LOG_INFO("Timeout");
//            return nullptr;
//        });
    }
};

int main (int argc, char **argv) {
//   HelloWorldApp app;
//   app.run(argc, argv);

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

//    Person p1{"testName", 21, "M", 167.5};
//    Person p2;
//
//    Archive<> a1;
//    Archive<ByteOrderNetwork> a2;
//    Archive<ByteOrderBigEndian> a3;
//    Archive<ByteOrderLittleEndian> a4;
//
//    std::cout << "Archive test start" << std::endl;
//    p1.display();
//
//    std::cout << "Archive Native" << std::endl;
//    a1 << p1;
//    a1 >> p2;
//    p2.display();
//
//    std::cout << "Archive Network" << std::endl;
//    a2 << p1;
//    a2 >> p2;
//    p2.display();
//
//    std::cout << "Archive BE" << std::endl;
//    a3 << p1;
//    a3 >> p2;
//    p2.display();
//
//    std::cout << "Archive LE" << std::endl;
//    a4 << p1;
//    a4 >> p2;
//    p2.display();

//    auto d1 = makeDeferred();
//    d1->addCallback([](DeferredValue value) {
//        std::cout << "success:" << *value.asValue<std::string>() << std::endl;
//        return std::string{"yyy"};
//    })->addErrback([](DeferredValue value) {
//        std::cout << "error:" << value.isError() << std::endl;
//        return nullptr;
//    })->addCallback([](DeferredValue value) {
//        std::cout << "success:" << *value.asValue<std::string>() << std::endl;
//        return 2;
//    });
//    d1->callback(std::string{"xxx"});

//    auto d2 = makeDeferred();
//    d2->addBoth([](DeferredValue value){
//        std::cout << "callback isError:" << value.isError() << std::endl;
//        try {
//            value.throwError();
//        } catch (std::exception &e) {
//            std::cout << e.what() << std::endl;
//        }
//        return value;
//    });
//    d2->errback(std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ValueError, "value error")));

//    DeferredPtr<int> d = makeDeferred<int>();
//    DeferredValue<int> x{d};
//    std::cout << "IsEmpty:" << x.isEmpty() << std::endl;
//    std::cout << "IsNull:" << x.isNull() << std::endl;
//    std::cout << "IsValue:" << x.isValue() << std::endl;
//    if (x.isValue()) {
//        std::cout << "Value:" << *x.asValue() << std::endl;
//        *x.asValue() = 20;
//        std::cout << "value2:" << *x.asValue() << std::endl;
//    }
//    std::cout << "IsError:" << x.isError() << std::endl;
//    if (x.isError()) {
//        try {
//            x.throwError();
//        } catch (std::exception &e) {
//            std::cout << "Error:" << e.what() << std::endl;
//        }
//    }
//    std::cout << "IsDeferred:" << x.isDeferred() << std::endl;
//    if (x.isDeferred()) {
//        std::cout << "UseCout:" << d.use_count() << std::endl;
//        d->result() = 10;
//        std::cout << "Value3:" << *x.asDeferred()->result().asValue() << std::endl;
//        x.throwError();
//        x.releaseDeferred();
//        std::cout << "UseCout:" << d.use_count() << std::endl;
//        d->result() = 30;
//        std::cout << "Value4:" << *x.asDeferred()->result().asValue() << std::endl;
//    }
//    std::cout << std::endl;

//    DeferredValue<void> dv2;
//    std::cout << "IsEmpty:" << dv2.isEmpty() << std::endl;
//    std::cout << "IsNull:" << dv2.isNull() << std::endl;
//    std::cout << "IsError:" << dv2.isError() << std::endl;
//    if (x.isError()) {
//        try {
//            x.throwError();
//        } catch (std::exception &e) {
//            std::cout << "Error:" << e.what() << std::endl;
//        }
//    }
//    std::cout << "IsDeferred:" << dv2.isDeferred() << std::endl;
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
