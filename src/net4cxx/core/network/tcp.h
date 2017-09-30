//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_TCP_H
#define NET4CXX_CORE_NETWORK_TCP_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/base.h"

NS_BEGIN

class Factory;
class ClientFactory;
class TCPConnector;

class NET4CXX_COMMON_API TCPConnection: public Connection {
public:
    using SocketType = boost::asio::ip::tcp::socket;

    TCPConnection(std::shared_ptr<Protocol> protocol, Reactor *reactor);

    SocketType& getSocket() {
        return _socket;
    }

    void write(const Byte *data, size_t length) override;

    void loseConnection() override;

    void abortConnection() override;

    bool getTcpNoDely() const {
        boost::asio::ip::tcp::no_delay option;
        _socket.get_option(option);
        return option.value();
    }

    void setTcpNoDelay(bool enabled) {
        boost::asio::ip::tcp::no_delay option(enabled);
        _socket.set_option(option);
    }

    bool getTcpKeepAlive() const {
        boost::asio::socket_base::keep_alive option;
        _socket.get_option(option);
        return option.value();
    }

    void setTcpKeepAlive(bool enabled) {
        boost::asio::socket_base::keep_alive option(enabled);
        _socket.set_option(option);
    }

    std::string getLocalAddress() const {
        auto endpoint = _socket.local_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getLocalPort() const {
        auto endpoint = _socket.local_endpoint();
        return endpoint.port();
    }

    std::string getRemoteAddress() const {
        auto endpoint = _socket.remote_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getRemotePort() const {
        auto endpoint = _socket.remote_endpoint();
        return endpoint.port();
    }
protected:
    void doClose();

    void doAbort();

    virtual void closeSocket();

    void startReading() {
        if (!_reading) {
            doRead();
        }
    }

    void doRead();

    void cbRead(const boost::system::error_code &ec, size_t transferredBytes) {
        _reading = false;
        handleRead(ec, transferredBytes);
        if (!_disconnecting && !_disconnected) {
            doRead();
        }
    }

    void handleRead(const boost::system::error_code &ec, size_t transferredBytes);

    void startWriting() {
        if (!_writing) {
            doWrite();
        }
    }

    void doWrite();

    void cbWrite(const boost::system::error_code &ec, size_t transferredBytes) {
        _writing = false;
        handleWrite(ec, transferredBytes);
        if (!_disconnecting && !_disconnected && !_writeQueue.empty()) {
            doWrite();
        }
    }

    void handleWrite(const boost::system::error_code &ec, size_t transferredBytes);

    SocketType _socket;
    std::exception_ptr _error;
};


class NET4CXX_COMMON_API TCPServerConnection: public TCPConnection {
public:
    explicit TCPServerConnection(Reactor *reactor)
            : TCPConnection({}, reactor) {
#ifndef NET4CXX_NDEBUG
        NET4CXX_Watcher->inc(NET4CXX_TCPSERVERCONNECTION_COUNT);
#endif
    }

#ifndef NET4CXX_NDEBUG
    ~TCPServerConnection() override {
        NET4CXX_Watcher->dec(NET4CXX_TCPSERVERCONNECTION_COUNT);
    }
#endif

    void cbAccept(std::shared_ptr<Protocol> protocol);
};


class NET4CXX_COMMON_API TCPClientConnection: public TCPConnection {
public:
    explicit TCPClientConnection(Reactor *reactor)
            : TCPConnection({}, reactor) {
#ifndef NET4CXX_NDEBUG
        NET4CXX_Watcher->inc(NET4CXX_TCPCLIENTCONNECTION_COUNT);
#endif
    }

#ifndef NET4CXX_NDEBUG
    ~TCPClientConnection() override {
        NET4CXX_Watcher->dec(NET4CXX_TCPCLIENTCONNECTION_COUNT);
    }
#endif

    void cbConnect(std::shared_ptr<Protocol> protocol, std::shared_ptr<TCPConnector> connector);
protected:
    void closeSocket() override;

    std::shared_ptr<TCPConnector> _connector;
};

class NET4CXX_COMMON_API TCPListener: public Listener {
public:
    using AddressType = boost::asio::ip::address;
    using AcceptorType = boost::asio::ip::tcp::acceptor;
    using SocketType = boost::asio::ip::tcp::socket;
    using ResolverType = boost::asio::ip::tcp::resolver ;
    using EndpointType = boost::asio::ip::tcp::endpoint ;
    using ResolverIterator = ResolverType::iterator;

    TCPListener(std::string port, std::unique_ptr<Factory> &&factory, std::string interface, Reactor *reactor);

#ifndef NET4CXX_NDEBUG
    ~TCPListener() override {
        NET4CXX_Watcher->dec(NET4CXX_TCPLISTENER_COUNT);
    }
#endif

    void startListening() override;

    void stopListening() override;

    std::string getLocalAddress() const {
        auto endpoint = _acceptor.local_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getLocalPort() const {
        auto endpoint = _acceptor.local_endpoint();
        return endpoint.port();
    }
protected:
    void cbAccept(const boost::system::error_code &ec);

    void handleAccept(const boost::system::error_code &ec);

    void doAccept() {
        _connection = std::make_shared<TCPServerConnection>(_reactor);
        _acceptor.async_accept(_connection->getSocket(), std::bind(&TCPListener::cbAccept, getSelf<TCPListener>(),
                                                                   std::placeholders::_1));
    }

    std::string _port;
    std::unique_ptr<Factory> _factory;
    std::string _interface;
    AcceptorType _acceptor;
    bool _connected{false};
    std::shared_ptr<TCPServerConnection> _connection;
};


class NET4CXX_COMMON_API TCPConnector: public Connector {
public:
    using AddressType = boost::asio::ip::address;
    using SocketType = boost::asio::ip::tcp::socket;
    using ResolverType = boost::asio::ip::tcp::resolver ;
    using ResolverIterator = ResolverType::iterator;
    using EndpointType = boost::asio::ip::tcp::endpoint ;

    TCPConnector(std::string host, std::string port, std::unique_ptr<ClientFactory> &&factory, double timeout,
                 Address bindAddress, Reactor *reactor);

#ifndef NET4CXX_NDEBUG
    ~TCPConnector() override {
        NET4CXX_Watcher->dec(NET4CXX_TCPCONNECTOR_COUNT);
    }
#endif

    void startConnecting() override;

    void stopConnecting() override;

    void connectionFailed(std::exception_ptr reason={});

    void connectionLost(std::exception_ptr reason={});
protected:
    std::shared_ptr<Protocol> buildProtocol(const Address &address);

    void cancelTimeout() {
        if (!_timeoutId.cancelled()) {
            _timeoutId.cancel();
        }
    }

    void doResolve();

    void cbResolve(const boost::system::error_code &ec, ResolverIterator iterator) {
        handleResolve(ec, iterator);
        if (_state == kConnecting) {
            doConnect(std::move(iterator));
        }
    }

    void handleResolve(const boost::system::error_code &ec, ResolverIterator iterator);

    void doConnect();

    void doConnect(ResolverIterator iterator);

    void cbConnect(const boost::system::error_code &ec) {
        handleConnect(ec);
    }

    void handleConnect(const boost::system::error_code &ec);

    void cbTimeout() {
        handleTimeout();
    }

    void handleTimeout();

    void makeTransport();

    enum State {
        kDisconnected,
        kConnecting,
        kConnected,
    };

    std::string _host;
    std::string _port;
    std::unique_ptr<ClientFactory> _factory;
    double _timeout{0.0};
    Address _bindAddress;
    ResolverType _resolver;
    std::shared_ptr<TCPClientConnection> _connection;
    State _state{kDisconnected};
    DelayedCall _timeoutId;
    bool _factoryStarted{false};
    std::exception_ptr _error;
};


NS_END

#endif //NET4CXX_CORE_NETWORK_TCP_H
