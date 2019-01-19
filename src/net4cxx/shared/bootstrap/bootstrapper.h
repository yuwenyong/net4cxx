//
// Created by yuwenyong.vincent on 2018/10/5.
//

#ifndef NET4CXX_SHARED_BOOTSTRAP_BOOTSTRAPPER_H
#define NET4CXX_SHARED_BOOTSTRAP_BOOTSTRAPPER_H

#include "net4cxx/common/common.h"
#include <boost/function.hpp>


NS_BEGIN

class Reactor;

class NET4CXX_COMMON_API Bootstrapper: public boost::noncopyable {
public:
    Bootstrapper();

    virtual ~Bootstrapper();

    void run(const boost::function1<std::string, std::string> &name_mapper={});

    void run(int argc, const char * const argv[]);

    void run(const char *path);

    void setCrashReportPath(const std::string &crashReportPath);

    void enableReactor(size_t numThreads=0) {
        _numThreads = numThreads;
    }

    void disableReactor() {
        _numThreads = boost::none;
    }

    Reactor* reactor() {
        return _reactor;
    }

    virtual void onPreInit();

    virtual void onInit();

    virtual void onRun();

    virtual void onQuit();

    static Bootstrapper* instance() {
        return _instance;
    }
protected:
    void cleanup();

    static Bootstrapper *_instance;

    bool _inited{false};
    boost::optional<size_t> _numThreads;
    Reactor *_reactor{nullptr};
};


class NET4CXX_COMMON_API BasicBootstrapper: public Bootstrapper {
public:
    void onPreInit() override;

protected:
    void setupCommonWatchObjects();
};


class NET4CXX_COMMON_API CommonBootstrapper: public BasicBootstrapper {
public:
    void onPreInit() override;
};


class NET4CXX_COMMON_API AppBootstrapper: public CommonBootstrapper {
public:
    void onInit() override;
};

NS_END


#define NET4CXX_WATCH_OBJECT(keyName) do { \
    NET4CXX_Watcher->addIncCallback(keyName, [](int oldValue, int increment, int value) { \
        NET4CXX_LOG_TRACE(net4cxx::gGenLog, "Increase %s, old value:%d, new value:%d", keyName, oldValue, value); \
    }); \
    NET4CXX_Watcher->addDecCallback(keyName, [](int oldValue, int decrement, int value) { \
        NET4CXX_LOG_TRACE(net4cxx::gGenLog, "Decrease %s, old value:%d, new value:%d", keyName, oldValue, value); \
    }); \
    NET4CXX_Watcher->addSetCallback(keyName, [](int oldValue, int value) { \
        NET4CXX_LOG_TRACE(net4cxx::gGenLog, "Update %s, old value:%d, new value:%d", keyName, oldValue, value); \
    }); \
} while (false)


#endif //NET4CXX_SHARED_BOOTSTRAP_BOOTSTRAPPER_H
