//
// Created by yuwenyong on 17-9-20.
//

#ifndef NET4CXX_CORE_NETWORK_BASE_H
#define NET4CXX_CORE_NETWORK_BASE_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/algorithm/string.hpp>
#include "net4cxx/common/utilities/errors.h"
#include "net4cxx/common/utilities/messagebuffer.h"

NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(AlreadyCancelled, Exception);
NET4CXX_DECLARE_EXCEPTION(ReactorNotRunning, Exception);
NET4CXX_DECLARE_EXCEPTION(ReactorAlreadyRunning, Exception);
NET4CXX_DECLARE_EXCEPTION(NotConnectingError, Exception);
NET4CXX_DECLARE_EXCEPTION(ConnectionDone, IOError);
NET4CXX_DECLARE_EXCEPTION(ConnectionAbort, IOError);
NET4CXX_DECLARE_EXCEPTION(UserAbort, Exception);


class Reactor;
class Protocol;

class NET4CXX_COMMON_API NetUtil {
public:
    static bool isValidIP(const std::string &ip) {
        boost::system::error_code ec;
        boost::asio::ip::address::from_string(ip, ec);
        return !ec;
    }

    static bool isValidPort(const std::string &port) {
        if (port.empty()) {
            return false;
        }
        return boost::all(port, boost::is_digit());
    }
};

class NET4CXX_COMMON_API Address {
public:
    Address() = default;

    Address(std::string address, unsigned short port)
            : _address{std::move(address)}
            , _port(port) {

    }

    void setAddress(std::string &&address) {
        _address = std::move(address);
    }

    void setAddress(const std::string &address) {
        _address = address;
    }

    const std::string& getAddress() const {
        return _address;
    }

    void setPort(unsigned short port) {
        _port = port;
    }

    unsigned short getPort() const {
        return _port;
    }
protected:
    std::string _address;
    unsigned short _port{0};
};


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
    DelayedCall() = default;

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


class NET4CXX_COMMON_API Connection: public std::enable_shared_from_this<Connection> {
public:
    Connection(std::shared_ptr<Protocol> protocol, Reactor *reactor)
            : _protocol(std::move(protocol))
            , _reactor(reactor) {

    }

    virtual ~Connection() = default;

    virtual void write(const Byte *data, size_t length) = 0;

    virtual void loseConnection() = 0;

    virtual void abortConnection() = 0;

    Reactor* reactor() {
        return _reactor;
    }

    template <typename ResultT>
    std::shared_ptr<ResultT> getSelf() {
        return std::static_pointer_cast<ResultT>(shared_from_this());
    }
protected:
    void dataReceived(Byte *data, size_t length);

    void connectionLost(std::exception_ptr reason);

    std::shared_ptr<Protocol> _protocol;
    Reactor *_reactor{nullptr};
    MessageBuffer _readBuffer;
    std::deque<MessageBuffer> _writeQueue;
    bool _reading{false};
    bool _writing{false};
    bool _connected{false};
    bool _disconnected{false};
    bool _disconnecting{false};
};

class NET4CXX_COMMON_API Listener: public std::enable_shared_from_this<Listener> {
public:
    explicit Listener(Reactor *reactor)
            : _reactor(reactor) {

    }

    virtual ~Listener() = default;

    virtual void startListening() = 0;

    virtual void stopListening() = 0;

    Reactor* reactor() {
        return _reactor;
    }

    template <typename ResultT>
    std::shared_ptr<ResultT> getSelf() {
        return std::static_pointer_cast<ResultT>(shared_from_this());
    }
protected:
    Reactor *_reactor{nullptr};
};


class NET4CXX_COMMON_API Connector: public std::enable_shared_from_this<Connector> {
public:
    explicit Connector(Reactor *reactor)
            : _reactor(reactor) {

    }

    virtual ~Connector() = default;

    virtual void startConnecting() = 0;

    virtual void stopConnecting() = 0;

    Reactor* reactor() {
        return _reactor;
    }

    template <typename ResultT>
    std::shared_ptr<ResultT> getSelf() {
        return std::static_pointer_cast<ResultT>(shared_from_this());
    }
protected:
    Reactor *_reactor{nullptr};
};

NS_END

#endif //NET4CXX_CORE_NETWORK_BASE_H
