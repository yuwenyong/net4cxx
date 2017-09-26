//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/tcp.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"

NS_BEGIN

TCPConnection::TCPConnection(Reactor *reactor)
        : Connection(reactor)
        , _socket(reactor->createSocket()) {

}

TCPConnection::TCPConnection(const std::weak_ptr<Protocol> &protocol, Reactor *reactor)
        : Connection(protocol, reactor)
        , _socket(reactor->createSocket()) {

}

const char* TCPConnection::logPrefix() const {
    return "TCPConnection";
}


TCPServerConnection::TCPServerConnection(unsigned int sessionno, Reactor *reactor)
        : TCPConnection(reactor)
        , _sessionno(sessionno) {
    _logstr = StrUtil::format("TCPServerConnection, %u", sessionno);
}

TCPServerConnection::TCPServerConnection(const std::weak_ptr<Protocol> &protocol, unsigned int sessionno,
                                         Reactor *reactor)
        : TCPConnection(protocol, reactor)
        , _sessionno(sessionno) {
    _logstr = StrUtil::format("TCPServerConnection, %u", sessionno);
}

const char* TCPServerConnection::logPrefix() const {
    return _logstr.c_str();
}


TCPPort::TCPPort(unsigned short port, std::unique_ptr<Factory> &&factory, std::string interface, Reactor *reactor)
        : Port(reactor)
        , _port(port)
        , _factory(std::move(factory))
        , _interface(std::move(interface))
        , _acceptor(reactor->createAcceptor()) {

}

const char* TCPPort::logPrefix() const {
    return "TCPPort";
}


void TCPPort::startListening() {
    ResolverType resolver(_reactor->getService());
    ResolverType::query query(_interface, std::to_string(_port));
    EndpointType endpoint = *resolver.resolve(query);
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen();
    NET4CXX_INFO(gGenLog, "%s starting on %u", logPrefix(), _port);
    _factory->doStart();
    _connected = true;
    doAccept();
}

void TCPPort::stopListening() {
    if (_connected) {
        _connected = false;
        _acceptor.close();
        _factory->doStop();
        NET4CXX_INFO(gGenLog, "%s closed on %u", logPrefix(), _port);
    }
}

void TCPPort::cbAccept(const boost::system::error_code &ec) {
    handleAccept(ec);
    if (!_connected) {
        return;
    }
    doAccept();
}

void TCPPort::handleAccept(const boost::system::error_code &ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_ERROR(gGenLog, "Could not accept new connection(%d:%s)", ec.value(), ec.message().c_str());
        }
        std::string address = _connection->getRemoteAddress();
        unsigned short port = _connection->getRemotePort();
        auto protocol = _factory->buildProtocol(address, port);
        if (protocol) {
            ++_sessionno;
            _connection->setProtocol(protocol);
            protocol->makeConnection(_connection);
            _connection->startReading();
        }
        _connection.reset();
    }
}

NS_END
