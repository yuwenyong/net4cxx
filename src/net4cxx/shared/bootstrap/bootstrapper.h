//
// Created by yuwenyong.vincent on 2018/10/5.
//

#ifndef NET4CXX_SHARED_BOOTSTRAP_BOOTSTRAPPER_H
#define NET4CXX_SHARED_BOOTSTRAP_BOOTSTRAPPER_H

#include "net4cxx/common/common.h"
#include <thread>
#include <boost/function.hpp>


NS_BEGIN

class Reactor;

class NET4CXX_COMMON_API Bootstrapper: public boost::noncopyable {
public:
    explicit Bootstrapper(bool commandThreadEnabled);

    explicit Bootstrapper(size_t numThreads=0, bool commandThreadEnabled=false);

    virtual ~Bootstrapper();

    void run(const boost::function1<std::string, std::string> &name_mapper={});

    void run(int argc, const char * const argv[]);

    void run(const char *path);

    void setCrashReportPath(const std::string &crashReportPath);

    Reactor* reactor() {
        return _reactor.get();
    }

    virtual void onPreInit();

    virtual void onInit();

    virtual void onRun();

    virtual void onQuit();

    virtual bool onSysCommand(const std::string &command);

    virtual bool onUserCommand(const std::string &command);

    static Bootstrapper* instance() {
        return _instance;
    }
protected:
    void doPreInit();

    void doInit();

    void doRun();

    void commandThread();

    bool onCommand(const std::string &command) {
        if (onSysCommand(command)) {
            return true;
        }
        return onUserCommand(command);
    }

    void cleanup();

    void setupCommonWatchObjects();

#if PLATFORM != PLATFORM_WINDOWS
    static int kbHitReturn();
#endif

    static void shutdownCommandThread(std::thread* commandThread);

    bool _inited{false};
    bool _commandThreadEnabled{false};
    volatile bool _commandThreadStopped{false};
    std::unique_ptr<Reactor> _reactor;

    static Bootstrapper *_instance;
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
