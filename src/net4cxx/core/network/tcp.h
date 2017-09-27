//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_TCP_H
#define NET4CXX_CORE_NETWORK_TCP_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include "net4cxx/core/network/base.h"

NS_BEGIN

class Factory;

class NET4CXX_COMMON_API TCPConnection: public Connection {
public:
    using SocketType = boost::asio::ip::tcp::socket;

    TCPConnection(const std::weak_ptr<Protocol> &protocol, Reactor *reactor);

    const char* logPrefix() const override;

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
    TCPServerConnection(unsigned int sessionno, Reactor *reactor);

    const char* logPrefix() const override;

    void cbAccept(const std::weak_ptr<Protocol> &protocol);
protected:
    unsigned int _sessionno;
    std::string _logstr;
};


class NET4CXX_COMMON_API TCPPort: public Port, public std::enable_shared_from_this<TCPPort> {
public:
    using AcceptorType = boost::asio::ip::tcp::acceptor;
    using SocketType = boost::asio::ip::tcp::socket;
    using ResolverType = boost::asio::ip::tcp::resolver ;
    using EndpointType = boost::asio::ip::tcp::endpoint ;
    using ResolverIterator = ResolverType::iterator;

    TCPPort(unsigned short port, std::unique_ptr<Factory> &&factory, std::string interface, Reactor *reactor);

    const char* logPrefix() const override;

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
        _connection = std::make_shared<TCPServerConnection>(_sessionno, _reactor);
        _acceptor.async_accept(_connection->getSocket(), std::bind(&TCPPort::cbAccept, shared_from_this(),
                                                                   std::placeholders::_1));
    }

    unsigned short _port{0};
    std::unique_ptr<Factory> _factory;
    std::string _interface;
    AcceptorType _acceptor;
    bool _connected{false};
    unsigned int _sessionno{0};
    std::shared_ptr<TCPServerConnection> _connection;
};

NS_END

#endif //NET4CXX_CORE_NETWORK_TCP_H
