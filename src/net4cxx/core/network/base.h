//
// Created by yuwenyong on 17-9-20.
//

#ifndef NET4CXX_CORE_NETWORK_BASE_H
#define NET4CXX_CORE_NETWORK_BASE_H

#include "net4cxx/common/common.h"
#include <boost/asio/steady_timer.hpp>
#include "net4cxx/common/utilities/messagebuffer.h"

NS_BEGIN

class Reactor;
class Protocol;

class NET4CXX_COMMON_API Timeout: public std::enable_shared_from_this<Timeout> {
public:
    friend Reactor;
    friend class DelayedCall;
    typedef boost::asio::steady_timer TimerType;

    explicit Timeout(Reactor *reactor);

    Timeout(const Timeout&) = delete;

    Timeout& operator=(const Timeout&) = delete;
protected:
    template <typename CallbackT>
    void start(const Timestamp &deadline, CallbackT &&callback) {
        _timer.expires_at(deadline);
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void start(const Duration &deadline, CallbackT &&callback) {
        _timer.expires_from_now(deadline);
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void start(double deadline, CallbackT &&callback) {
        _timer.expires_from_now(std::chrono::milliseconds(int64_t(deadline * 1000)));
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void wait(CallbackT &&callback) {
        _timer.async_wait([callback = std::forward<CallbackT>(callback), timeout = shared_from_this()](
                const boost::system::error_code &ec) {
            if (!ec) {
                callback();
            }
        });
    }

    void cancel() {
        _timer.cancel();
    }

    TimerType _timer;
};

typedef std::weak_ptr<Timeout> TimeoutHandle;

class NET4CXX_COMMON_API DelayedCall {
public:
    explicit DelayedCall(TimeoutHandle timeout)
            : _timeout(std::move(timeout)) {

    }

    bool cancelled() const {
        return _timeout.expired();
    }

    void cancel();
protected:
    TimeoutHandle _timeout;
};


class NET4CXX_COMMON_API Port {
public:
    explicit Port(Reactor *reactor)
            : _reactor(reactor) {

    }

    virtual ~Port() = default;

    virtual const char* logPrefix() const;

    virtual void startListening() = 0;

    virtual void stopListening() = 0;
protected:
    Reactor *_reactor{nullptr};
};


class NET4CXX_COMMON_API Connection {
public:
    Connection(const std::weak_ptr<Protocol> &protocol, Reactor *reactor)
            : _protocol(protocol)
            , _reactor(reactor) {

    }

    virtual ~Connection() = default;

    virtual const char* logPrefix() const;

    virtual void write(const Byte *data, size_t length) = 0;

    virtual void loseConnection() = 0;

    virtual void abortConnection() = 0;
protected:
    void dataReceived(Byte *data, size_t length);

    void connectionLost(std::exception_ptr reason);

    std::weak_ptr<Protocol> _protocol;
    Reactor *_reactor{nullptr};
    MessageBuffer _readBuffer;
    std::deque<MessageBuffer> _writeQueue;
    bool _reading{false};
    bool _writing{false};
    bool _connected{false};
    bool _disconnected{false};
    bool _disconnecting{false};
};

NS_END

#endif //NET4CXX_CORE_NETWORK_BASE_H
