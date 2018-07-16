//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/tcp.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"

NS_BEGIN


TCPConnection::TCPConnection(const ProtocolPtr &protocol, Reactor *reactor)
        : Connection(protocol, reactor)
        , _socket(reactor->getIOContext()) {

}

void TCPConnection::write(const Byte *data, size_t length) {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    if (!length) {
        return;
    }
    MessageBuffer packet(length);
    packet.write(data, length);
    _writeQueue.emplace_back(std::move(packet));
    startWriting();
}

void TCPConnection::loseConnection() {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionDone, ""));
    _disconnecting = true;
    doClose();
}

void TCPConnection::abortConnection() {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionAbort, ""));
    _disconnecting = true;
    _aborting = true;
    doAbort();
}

void TCPConnection::doClose() {
    if (!_writing && !_reading) {
        _reactor->addCallback([this, self=shared_from_this()]() {
            if (!_disconnected) {
                closeSocket();
            }
        });
    } else if (!_writing) {
        _socket.close();
    }
}

void TCPConnection::doAbort() {
    if (!_writing && !_reading) {
        _reactor->addCallback([this, self=shared_from_this()]() {
            if (!_disconnected) {
                closeSocket();
            }
        });
    } else {
        _socket.close();
    }
}

void TCPConnection::closeSocket() {
    _connected = false;
    _disconnected = true;
    _disconnecting = false;
    _aborting = false;
    if (_socket.is_open()) {
        _socket.close();
    }
    connectionLost(_error);
}

void TCPConnection::doRead() {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    _readBuffer.normalize();
    _readBuffer.ensureFreeSpace();
    _reading = true;
    _socket.async_read_some(boost::asio::buffer(_readBuffer.getWritePointer(), _readBuffer.getRemainingSpace()),
                            [protocol, self = shared_from_this()](const boost::system::error_code &ec,
                                                                  size_t transferredBytes) {
                                self->cbRead(ec, transferredBytes);
                            });
}

void TCPConnection::handleRead(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {
            NET4CXX_LOG_ERROR(gGenLog, "Read error %d :%s", ec.value(), ec.message().c_str());
        }
        if (!_disconnected) {
            if (ec == boost::asio::error::operation_aborted) {
                NET4CXX_ASSERT(_error);
            } else if (ec == boost::asio::error::eof) {
                _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionDone, ""));
            } else {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
            }
            closeSocket();
        }
    } else {
        _readBuffer.writeCompleted(transferredBytes);
        if (_disconnecting) {
            return;
        }
        dataReceived(_readBuffer.getReadPointer(), _readBuffer.getActiveSize());
        _readBuffer.readCompleted(_readBuffer.getActiveSize());
    }
}

void TCPConnection::doWrite() {
#ifndef BOOST_ASIO_HAS_IOCP
    size_t bytesToSend, bytesSent;
    boost::system::error_code ec;
    for(;;) {
        MessageBuffer &buffer = _writeQueue.front();
        bytesToSend = buffer.getActiveSize();
        bytesSent = _socket.write_some(boost::asio::buffer(buffer.getReadPointer(), bytesToSend), ec);
        if (ec) {
            if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again) {
                break;
            } else {
                NET4CXX_LOG_ERROR(gGenLog, "Write error %d :%s", ec.value(), ec.message().c_str());
            }
            _error = std::make_exception_ptr(boost::system::system_error(ec));
            _disconnecting = true;
            doClose();
            return;
        } else if (bytesSent == 0){
            _disconnecting = true;
            doClose();
            return;
        } else if (bytesSent < bytesToSend) {
            buffer.readCompleted(bytesSent);
            break;
        }
        _writeQueue.pop_front();
        if (_writeQueue.empty()) {
            return;
        }
    }
#endif
    MessageBuffer &buffer = _writeQueue.front();
#ifdef BOOST_ASIO_HAS_IOCP
    if (_writeQueue.size() > 1) {
        size_t space = 0;
        for (auto iter = std::next(_writeQueue.begin()); iter != _writeQueue.end(); ++iter) {
            space += iter->getActiveSize();
        }
        buffer.ensureFreeSpace(space);
        for (auto iter = std::next(_writeQueue.begin()); iter != _writeQueue.end(); ++iter) {
            buffer.write(iter->getReadPointer(), iter->getActiveSize());
        }
        while (_writeQueue.size() > 1) {
            _writeQueue.pop_back();
        }
    }
#endif
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    _writing = true;
    _socket.async_write_some(boost::asio::buffer(buffer.getReadPointer(), buffer.getActiveSize()),
                             [protocol, self = shared_from_this()](const boost::system::error_code &ec,
                                                                   size_t transferredBytes) {
                                 self->cbWrite(ec, transferredBytes);
                             });
}

void TCPConnection::handleWrite(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_ERROR(gGenLog, "Write error %d :%s", ec.value(), ec.message().c_str());
        }
        if (!_disconnected) {
            if (ec == boost::asio::error::operation_aborted) {
                NET4CXX_ASSERT(_error);
            } else {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
            }
            closeSocket();
        }
    } else {
        if (transferredBytes > 0) {
            _writeQueue.front().readCompleted(transferredBytes);
            if (!_writeQueue.front().getActiveSize()) {
                _writeQueue.pop_front();
            }
        }
        if ((_disconnecting && _writeQueue.empty()) || _aborting) {
            closeSocket();
        }
    }
}


void TCPServerConnection::cbAccept(const ProtocolPtr &protocol) {
    _protocol = protocol;
    _connected = true;
    _socket.non_blocking(true);
    protocol->makeConnection(shared_from_this());
    if (!_disconnecting) {
        startReading();
    }
}


void TCPClientConnection::cbConnect(const ProtocolPtr &protocol, std::shared_ptr<TCPConnector> connector) {
    _protocol = protocol;
    _connector = std::move(connector);
    _connected = true;
    _socket.non_blocking(true);
    protocol->makeConnection(shared_from_this());
    if (!_disconnecting) {
        startReading();
    }
}

void TCPClientConnection::closeSocket() {
    TCPConnection::closeSocket();
    _connector->connectionLost(_error);
}


TCPListener::TCPListener(std::string port, std::shared_ptr<Factory> factory, std::string interface,
                         Reactor *reactor)
        : Listener(reactor)
        , _port(std::move(port))
        , _factory(std::move(factory))
        , _interface(std::move(interface))
        , _acceptor(reactor->getIOContext()) {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(NET4CXX_TCPListener_COUNT);
#endif
    if (_interface.empty()) {
//        _interface = "::";
        _interface = "0.0.0.0";
    }
}

void TCPListener::startListening() {
    EndpointType endpoint;
    if (NetUtil::isValidIP(_interface) && NetUtil::isValidPort(_port)) {
        endpoint = {boost::asio::ip::make_address(_interface), (unsigned short)std::stoul(_port)};
    } else {
        ResolverType resolver(_reactor->getIOContext());
        ResolverType::results_type results = resolver.resolve(_interface, _port);
        endpoint = (*results.cbegin()).endpoint();
    }
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen();
    NET4CXX_LOG_INFO(gGenLog, "TCPListener starting on %s", _port.c_str());
    _factory->doStart();
    _connected = true;
    doAccept();
}

void TCPListener::stopListening() {
    if (_connected) {
        _connected = false;
        _acceptor.close();
        _factory->doStop();
        NET4CXX_LOG_INFO(gGenLog, "TCPListener closed on %s", _port.c_str());
    }
}

void TCPListener::cbAccept(const boost::system::error_code &ec) {
    handleAccept(ec);
    if (!_connected) {
        return;
    }
    doAccept();
}

void TCPListener::handleAccept(const boost::system::error_code &ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_ERROR(gGenLog, "Accept error %d: %s", ec.value(), ec.message().c_str());
        }
    } else {
        Address address{_connection->getRemoteAddress(), _connection->getRemotePort()};
        auto protocol = _factory->buildProtocol(address);
        if (protocol) {
            protocol->setFactory(_factory);
            _connection->cbAccept(protocol);
        }
    }
    _connection.reset();
}


TCPConnector::TCPConnector(std::string host, std::string port, std::shared_ptr<ClientFactory> factory, double timeout,
                           Address bindAddress, Reactor *reactor)
        : Connector(reactor)
        , _host(std::move(host))
        , _port(std::move(port))
        , _factory(std::move(factory))
        , _timeout(timeout)
        , _bindAddress(std::move(bindAddress))
        , _resolver(reactor->getIOContext()) {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(NET4CXX_TCPConnector_COUNT);
#endif
}

void TCPConnector::startConnecting() {
    if (_state != kDisconnected) {
        NET4CXX_THROW_EXCEPTION(Exception, "Can't connect in this state");
    }
    _state = kConnecting;
    if (!_factoryStarted) {
        _factory->doStart();
        _factoryStarted = true;
    }
    if (NetUtil::isValidIP(_host) && NetUtil::isValidPort(_port)) {
        doConnect();
    } else {
        doResolve();
    }
    if (_timeout != 0.0) {
        _timeoutId = _reactor->callLater(_timeout, [this, self=shared_from_this()]() {
            cbTimeout();
        });
    }
    _factory->startedConnecting(shared_from_this());
}

void TCPConnector::stopConnecting() {
    if (_state != kConnecting) {
        NET4CXX_THROW_EXCEPTION(NotConnectingError, "We're not trying to connect");
    }
    _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(UserAbort, ""));
    if (_connection) {
        _connection->getSocket().close();
        _connection.reset();
    } else {
        _resolver.cancel();
    }
    _state = kDisconnected;
}

void TCPConnector::connectionFailed(std::exception_ptr reason) {
    if (reason) {
        _error = reason;
    }
    cancelTimeout();
    _connection.reset();
    _state = kDisconnected;
    _factory->clientConnectionFailed(shared_from_this(), _error);
    if (_state == kDisconnected) {
        _factory->doStop();
        _factoryStarted = false;
    }
}

void TCPConnector::connectionLost(std::exception_ptr reason) {
    if (reason) {
        _error = reason;
    }
    _state = kDisconnected;
    _factory->clientConnectionLost(shared_from_this(), _error);
    if (_state == kDisconnected) {
        _factory->doStop();
        _factoryStarted = false;
    }
}

ProtocolPtr TCPConnector::buildProtocol(const Address &address) {
    _state = kConnected;
    cancelTimeout();
    return _factory->buildProtocol(address);
}

void TCPConnector::doResolve() {
    _resolver.async_resolve(_host, _port, [this, self=shared_from_this()](
            const boost::system::error_code &ec, ResolverResultsType results) {
        cbResolve(ec, results);
    });
}

void TCPConnector::handleResolve(const boost::system::error_code &ec, const ResolverResultsType &results) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_ERROR(gGenLog, "Resolve error %d :%s", ec.value(), ec.message().c_str());
            _error = std::make_exception_ptr(boost::system::system_error(ec));
        }
        connectionFailed();
    }
}

void TCPConnector::doConnect() {
    makeTransport();
    EndpointType endpoint{boost::asio::ip::make_address(_host), (unsigned short)std::stoul(_port)};
    _connection->getSocket().async_connect(endpoint, [this, self=shared_from_this(), connection=_connection](
            const boost::system::error_code &ec) {
        cbConnect(ec);
    });
}

void TCPConnector::doConnect(const ResolverResultsType &results) {
    makeTransport();
    boost::asio::async_connect(_connection->getSocket(), results, [this, self=shared_from_this(),
            connection=_connection](const boost::system::error_code &ec, const EndpointType &endpoint) {
        cbConnect(ec);
    });
}

void TCPConnector::handleConnect(const boost::system::error_code &ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_ERROR(gGenLog, "Connect error %d :%s", ec.value(), ec.message().c_str());
            _error = std::make_exception_ptr(boost::system::system_error(ec));
        }
        connectionFailed();
    } else {
        Address address{_connection->getRemoteAddress(), _connection->getRemotePort()};
        auto protocol = buildProtocol(address);
        if (!protocol) {
            _connection.reset();
            connectionLost();
        } else {
            protocol->setFactory(_factory);
            auto connection = std::move(_connection);
            connection->cbConnect(protocol, shared_from_this());
        }
    }
}

void TCPConnector::handleTimeout() {
    NET4CXX_LOG_ERROR(gGenLog, "Connect error : Timeout");
    _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(TimeoutError, ""));
    connectionFailed();
}

void TCPConnector::makeTransport() {
    _connection = std::make_shared<TCPClientConnection>(_reactor);
    if (!_bindAddress.getAddress().empty()) {
        EndpointType endpoint{boost::asio::ip::make_address(_bindAddress.getAddress()), _bindAddress.getPort()};
        _connection->getSocket().open(endpoint.protocol());
        _connection->getSocket().bind(endpoint);
    }
}

NS_END
