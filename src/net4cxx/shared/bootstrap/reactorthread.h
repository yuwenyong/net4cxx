//
// Created by yuwenyong.vincent on 2019-02-10.
//

#ifndef NET4CXX_SHARED_BOOTSTRAP_REACTORTHREAD_H
#define NET4CXX_SHARED_BOOTSTRAP_REACTORTHREAD_H

#include "net4cxx/common/common.h"
#include "net4cxx/core/network/reactor.h"
#include <thread>

NS_BEGIN


class NET4CXX_COMMON_API ReactorThread {
public:
    explicit ReactorThread(size_t numThreads=0): _reactor(numThreads) {}

    virtual ~ReactorThread();

    bool start(bool installSignalHandlers=false);

    void stop() {
        if (_thread) {
            _reactor.addCallback([this]{
                _reactor.stop();
            });
        }
    }

    void wait() {
        if (_thread) {
            _thread->join();
            _thread.reset();
        }
    }

    bool running() const {
        return (bool)_thread;
    }

    template <typename CallbackT>
    void addCallback(CallbackT &&callback) {
        _reactor.addCallback(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void addSubCallback(CallbackT &&callback) {
        selectReactor()->addCallback(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    bool addSubCallback(size_t index, CallbackT &&callback) {
        if (Reactor *reactor = getSubReactor(index)) {
            reactor->addCallback(std::forward<CallbackT>(callback));
            return true;
        } else {
            return false;
        }
    }

    virtual void onPreInit();

    virtual void onInit();

    virtual void onRun();

    virtual void onQuit();

    Reactor* reactor() {
        return &_reactor;
    }

    Reactor* selectReactor() {
        return _reactor.selectReactor();
    }

    size_t getSubReactorCount() const {
        return _reactor.getSubReactorCount();
    }

    Reactor* getSubReactor(size_t index) {
        return _reactor.getSubReactor(index);
    }
protected:
    Reactor _reactor;
    std::unique_ptr<std::thread> _thread;
};

NS_END

#endif //NET4CXX_SHARED_BOOTSTRAP_REACTORTHREAD_H
