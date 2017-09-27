//
// Created by yuwenyong on 17-9-20.
//

#ifndef NET4CXX_CORE_NETWORK_REACTOR_H
#define NET4CXX_CORE_NETWORK_REACTOR_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include "net4cxx/core/network/base.h"


NS_BEGIN

class Port;
class Factory;

class NET4CXX_COMMON_API Reactor {
public:
    using ServiceType = boost::asio::io_service;
    using WorkType = ServiceType::work;
    using AcceptorType = boost::asio::ip::tcp::acceptor;
    using SocketType = boost::asio::ip::tcp::socket;
    using SignalSet = boost::asio::signal_set;
    using StopCallbacks = boost::signals2::signal<void ()>;

    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    Reactor();

    ~Reactor() = default;

    void makeCurrent() {
        _current = this;
    }

    void run(bool installSignalHandlers=true);

    template <typename CallbackT>
    DelayedCall callLater(double deadline, CallbackT &&callback) {
        auto timeout = std::make_shared<Timeout>(this);
        timeout->start(deadline, std::forward<CallbackT>(callback));
        return DelayedCall(timeout);
    }

    template <typename CallbackT>
    DelayedCall callLater(const Duration &deadline, CallbackT &&callback) {
        auto timeout = std::make_shared<Timeout>(this);
        timeout->start(deadline, std::forward<CallbackT>(callback));
        return DelayedCall(timeout);
    }

    template <typename CallbackT>
    DelayedCall callAt(time_t deadline, CallbackT &&callback) {
        auto timeout = std::make_shared<Timeout>(this);
        timeout->start(Timestamp{std::chrono::seconds(deadline)}, std::forward<CallbackT>(callback));
        return DelayedCall(timeout);
    }

    template <typename CallbackT>
    DelayedCall callAt(const Timestamp &deadline, CallbackT &&callback) {
        auto timeout = std::make_shared<Timeout>(this);
        timeout->start(deadline, std::forward<CallbackT>(callback));
        return DelayedCall(timeout);
    }

    template <typename CallbackT>
    void addCallback(CallbackT &&callback) {
        _ioService.post(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void addStopCallback(CallbackT &&callback) {
        _stopCallbacks.connect(std::forward<CallbackT>(callback));
    }

    AcceptorType createAcceptor() {
        return AcceptorType{_ioService};
    }

    SocketType createSocket() {
        return SocketType{_ioService};
    }

    std::shared_ptr<Port> listenTCP(unsigned short port, std::unique_ptr<Factory> &&factory,
                                    const std::string &interface);

    bool running() const {
        return !_running;
    }

    ServiceType& getService() {
        return _ioService;
    }

    void stop();

    static Reactor *current() {
        return _current;
    }

    static void clearCurrent() {
        _current = nullptr;
    }
protected:
    void startRunning(bool installSignalHandlers=true);

    void handleSignals();

    void onSignal(const boost::system::error_code &ec, int signalNumber);

    void sigInt();

    void sigTerm();

    void sigBreak();

    void sigQuit();

    ServiceType _ioService;
    SignalSet _signalSet;
    bool _installSignalHandlers{false};
    volatile bool _running{false};
    StopCallbacks _stopCallbacks;
    thread_local static Reactor *_current;
};

NS_END

#endif //NET4CXX_CORE_NETWORK_REACTOR_H
