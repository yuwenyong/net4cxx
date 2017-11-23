//
// Created by yuwenyong on 17-9-20.
//

#ifndef NET4CXX_CORE_NETWORK_REACTOR_H
#define NET4CXX_CORE_NETWORK_REACTOR_H

#include "net4cxx/common/common.h"
#include <boost/signals2.hpp>
#include "net4cxx/core/network/base.h"
#include "net4cxx/core/network/resolver.h"


NS_BEGIN


class Factory;
class ClientFactory;


class NET4CXX_COMMON_API Reactor {
public:
    using ServiceType = boost::asio::io_service;
    using WorkType = ServiceType::work;
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

    ListenerPtr listenTCP(const std::string &port, std::unique_ptr<Factory> &&factory, const std::string &interface={});

    ConnectorPtr connectTCP(const std::string &host, const std::string &port, std::unique_ptr<ClientFactory> &&factory,
                            double timeout=30.0, const Address &bindAddress={});

    ListenerPtr listenSSL(const std::string &port, std::unique_ptr<Factory> &&factory, SSLOptionPtr sslOption,
                          const std::string &interface={});

    ConnectorPtr connectSSL(const std::string &host, const std::string &port, std::unique_ptr<ClientFactory> &&factory,
                            SSLOptionPtr sslOption, double timeout=30.0, const Address &bindAddress={});

    DatagramConnectionPtr listenUDP(unsigned short port, DatagramProtocolPtr protocol, const std::string &interface="",
                                    size_t maxPacketSize=8192);

    DatagramConnectionPtr connectUDP(const std::string &address, unsigned short port, DatagramProtocolPtr protocol,
                                     size_t maxPacketSize=8192, const Address &bindAddress={});

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    ListenerPtr listenUNIX(const std::string &path, std::unique_ptr<Factory> &&factory);

    ConnectorPtr connectUNIX(const std::string &path, std::unique_ptr<ClientFactory> &&factory, double timeout=30.0);
#endif

    bool running() const {
        return !_running;
    }

    ServiceType& getService() {
        return _ioService;
    }

    template <typename CallbackT>
    DelayedResolve resolve(const std::string &name, CallbackT &&callback) {
        auto resolver = std::make_shared<Resolver>(this);
        resolver->start(name, std::forward<CallbackT>(callback));
        return DelayedResolve(resolver);
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
