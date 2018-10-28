//
// Created by yuwenyong on 17-10-10.
//

#include "net4cxx/core/network/ssl.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/defer.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN


SSLConnection::SSLConnection(const ProtocolPtr &protocol, SSLOptionPtr sslOption, Reactor *reactor)
        : Connection(protocol, reactor)
        , _sslOption(std::move(sslOption))
        , _socket(reactor->getIOContext(), _sslOption->context()) {

}

void SSLConnection::write(const Byte *data, size_t length) {
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

void SSLConnection::loseConnection() {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionDone, ""));
    _disconnecting = true;
    doClose();
}

void SSLConnection::abortConnection() {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionAbort, ""));
    _disconnecting = true;
    _aborting = true;
    doAbort();
}

void SSLConnection::doClose() {
    if (_sslAccepted) {
        if (!_writing) {
            startShutdown();
        }
    } else {
        if (_sslAccepting) {
            _socket.lowest_layer().cancel();
        }
        auto protocol = _protocol.lock();
        NET4CXX_ASSERT(protocol);
        _reactor->addCallback([this, protocol, self=shared_from_this()]() {
            if (!_disconnected) {
                closeSocket();
            }
        });
    }
}

void SSLConnection::doAbort() {
    if (_sslAccepted) {
        if (_writing) {
            _socket.lowest_layer().cancel();
        }
        startShutdown();
    } else {
        if (_sslAccepting) {
            _socket.lowest_layer().cancel();
        }
        auto protocol = _protocol.lock();
        NET4CXX_ASSERT(protocol);
        _reactor->addCallback([this, protocol, self=shared_from_this()]() {
            if (!_disconnected) {
                closeSocket();
            }
        });
    }
}

void SSLConnection::closeSocket() {
    _connected = false;
    _disconnected = true;
    _disconnecting = false;
    _aborting = false;
    if (_socket.lowest_layer().is_open()) {
        _socket.lowest_layer().close();
    }
    connectionLost(_error);
}

void SSLConnection::doHandshake() {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    _sslAccepting = true;
    if (_sslOption->isServerSide()) {
        _socket.async_handshake(boost::asio::ssl::stream_base::server,
                                [protocol, self = shared_from_this()](const boost::system::error_code &ec) {
                                    self->cbHandshake(ec);
                                });
    } else {
        _socket.async_handshake(boost::asio::ssl::stream_base::client,
                                [protocol, self = shared_from_this()](const boost::system::error_code &ec) {
                                    self->cbHandshake(ec);
                                });
    }
}

void SSLConnection::handleHandshake(const boost::system::error_code &ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_ERROR(gGenLog, "Handshake error %d :%s", ec.value(), ec.message().c_str());
        }
        if (!_disconnected) {
            if (ec != boost::asio::error::operation_aborted) {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
            }
            closeSocket();
        }
    } else {
        _sslAccepted = true;
    }
}

void SSLConnection::doRead() {
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

void SSLConnection::handleRead(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof &&
            (ec.category() != boost::asio::error::get_ssl_category() ||
             ERR_GET_REASON(ec.value()) != SSL_R_SHORT_READ)) {
            NET4CXX_LOG_ERROR(gGenLog, "Read error %d :%s", ec.value(), ec.message().c_str());
        }
        if (!_disconnected) {
            if (ec == boost::asio::error::operation_aborted) {
                NET4CXX_ASSERT(_error);
            } else if (ec == boost::asio::error::eof) {
                _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionDone, ""));
            } else if (ec.category() != boost::asio::error::get_ssl_category() ||
                       ERR_GET_REASON(ec.value()) != SSL_R_SHORT_READ) {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
            } else {
                _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionDone, "SSL Short Read"));
            }
            _disconnecting = true;
            startShutdown();
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

void SSLConnection::doWrite() {
    MessageBuffer &buffer = _writeQueue.front();
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
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    _writing = true;
    _socket.async_write_some(boost::asio::buffer(buffer.getReadPointer(), buffer.getActiveSize()),
                             [protocol, self = shared_from_this()](const boost::system::error_code &ec,
                                                                   size_t transferredBytes) {
                                 self->cbWrite(ec, transferredBytes);
                             });
}

void SSLConnection::handleWrite(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof &&
            (ec.category() != boost::asio::error::get_ssl_category() ||
             ERR_GET_REASON(ec.value()) != SSL_R_SHORT_READ)) {
            NET4CXX_LOG_ERROR(gGenLog, "Write error %d :%s", ec.value(), ec.message().c_str());
        }
        if (!_disconnected) {
            if (ec == boost::asio::error::operation_aborted) {
                NET4CXX_ASSERT(_error);
            } else if (ec == boost::asio::error::eof) {
                _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionDone, ""));
            } else if (ec.category() != boost::asio::error::get_ssl_category() ||
                       ERR_GET_REASON(ec.value()) != SSL_R_SHORT_READ) {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
            } else {
                _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectionDone, "SSL Short Read"));
            }
            _disconnecting = true;
            startShutdown();
        }
    } else {
        if (transferredBytes > 0) {
            _writeQueue.front().readCompleted(transferredBytes);
            if (!_writeQueue.front().getActiveSize()) {
                _writeQueue.pop_front();
            }
        }
        if ((_disconnecting && _writeQueue.empty()) || _aborting) {
            startShutdown();
        }
    }
}

void SSLConnection::doShutdown() {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    _sslShutting = true;
    _socket.async_shutdown([protocol, self = shared_from_this()](const boost::system::error_code &ec) {
        self->cbShutdown(ec);
    });
}

void SSLConnection::handleShutdown(const boost::system::error_code &ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof &&
            (ec.category() != boost::asio::error::get_ssl_category() ||
             ERR_GET_REASON(ec.value()) != SSL_R_SHORT_READ)) {
            NET4CXX_LOG_ERROR(gGenLog, "Read error %d :%s", ec.value(), ec.message().c_str());
            _error = std::make_exception_ptr(boost::system::system_error(ec));
        }
    }
    NET4CXX_ASSERT(!_disconnected);
    closeSocket();
}


void SSLServerConnection::cbAccept(const ProtocolPtr &protocol) {
    _protocol = protocol;
    _connected = true;
    protocol->makeConnection(shared_from_this());
    if (!_disconnecting) {
        startHandshake();
    }
}


void SSLClientConnection::cbConnect(const ProtocolPtr &protocol, std::shared_ptr<SSLConnector> connector) {
    _protocol = protocol;
    _connector = std::move(connector);
    _connected = true;
    protocol->makeConnection(shared_from_this());
    if (!_disconnecting) {
        startHandshake();
    }
}

void SSLClientConnection::closeSocket() {
    SSLConnection::closeSocket();
    _connector->connectionLost(_error);
}


SSLListener::SSLListener(std::string port, std::shared_ptr<Factory> factory, SSLOptionPtr sslOption,
                         std::string interface, Reactor *reactor)
        : Listener(reactor)
        , _port(std::move(port))
        , _factory(std::move(factory))
        , _sslOption(std::move(sslOption))
        , _interface(std::move(interface))
        , _acceptor(reactor->getIOContext()) {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(WatchKeys::SSLListenerCount);
#endif
    if (_interface.empty()) {
//        _interface = "::";
        _interface = "0.0.0.0";
    }
}

void SSLListener::startListening() {
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
    NET4CXX_LOG_INFO(gGenLog, "SSLListener starting on %s", _port.c_str());
    _factory->doStart();
    _connected = true;
    _disconnected = false;
    doAccept();
}

DeferredPtr SSLListener::stopListening() {
    _disconnecting = true;
    if (_connected) {
        _deferred = makeDeferred();
        _acceptor.close();
        return _deferred;
    }
    return nullptr;
}

void SSLListener::connectionLost() {
    NET4CXX_LOG_INFO(gGenLog, "SSLListener closed on %s", _port.c_str());
    auto d = std::move(_deferred);
    _disconnected = true;
    _connected = false;
    try {
        _factory->doStop();
    } catch (...) {
        _disconnecting = false;
        if (d) {
            d->errback();
        } else {
            throw;
        }
    }
    if (_disconnecting) {
        _disconnecting = false;
        if (d) {
            d->callback(nullptr);
        }
    }
}

void SSLListener::cbAccept(const boost::system::error_code &ec) {
    handleAccept(ec);
    if (!_connected) {
        return;
    }
    doAccept();
}

void SSLListener::handleAccept(const boost::system::error_code &ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_ERROR(gGenLog, "Accept error %d: %s", ec.value(), ec.message().c_str());
        } else {
            connectionLost();
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


SSLConnector::SSLConnector(std::string host, std::string port, std::shared_ptr<ClientFactory> factory,
                           SSLOptionPtr sslOption, double timeout, Address bindAddress, Reactor *reactor)
        : Connector(reactor)
        , _host(std::move(host))
        , _port(std::move(port))
        , _factory(std::move(factory))
        , _sslOption(std::move(sslOption))
        , _timeout(timeout)
        , _bindAddress(std::move(bindAddress))
        , _resolver(reactor->getIOContext()) {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(WatchKeys::SSLConnectorCount);
#endif
}

void SSLConnector::startConnecting() {
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

void SSLConnector::stopConnecting() {
    if (_state != kConnecting) {
        NET4CXX_THROW_EXCEPTION(NotConnectingError, "We're not trying to connect");
    }
    _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(UserAbort, ""));
    abortConnecting();
}

void SSLConnector::connectionFailed(std::exception_ptr reason) {
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

void SSLConnector::connectionLost(std::exception_ptr reason) {
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

ProtocolPtr SSLConnector::buildProtocol(const Address &address) {
    _state = kConnected;
    cancelTimeout();
    return _factory->buildProtocol(address);
}

void SSLConnector::doResolve() {
    ResolverType::query query(_host, _port);
    _resolver.async_resolve(query, [this, self=shared_from_this()](
            const boost::system::error_code &ec, ResolverResultsType results) {
        cbResolve(ec, results);
    });
}

void SSLConnector::handleResolve(const boost::system::error_code &ec, const ResolverResultsType &results) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_ERROR(gGenLog, "Resolve error %d :%s", ec.value(), ec.message().c_str());
            _error = std::make_exception_ptr(boost::system::system_error(ec));
        }
        connectionFailed();
    }
}

void SSLConnector::doConnect() {
    makeTransport();
    EndpointType endpoint{boost::asio::ip::make_address(_host), (unsigned short)std::stoul(_port)};
    _connection->getSocket().lowest_layer().async_connect(
            endpoint, [this, self = shared_from_this(), connection = _connection](
                    const boost::system::error_code &ec) {
                cbConnect(ec);
            });
}

void SSLConnector::doConnect(const ResolverResultsType &results) {
    makeTransport();
    boost::asio::async_connect(_connection->getSocket().lowest_layer(), results,
                               [this, self = shared_from_this(), connection = _connection](
                                       const boost::system::error_code &ec, const EndpointType &endpoint) {
                                   cbConnect(ec);
                               });
}

void SSLConnector::handleConnect(const boost::system::error_code &ec) {
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

void SSLConnector::handleTimeout() {
    NET4CXX_ASSERT(_state == kConnecting);
    NET4CXX_LOG_ERROR(gGenLog, "Connect error : Timeout");
    _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(TimeoutError, ""));
    abortConnecting();
}

void SSLConnector::makeTransport() {
    _connection = std::make_shared<SSLClientConnection>(_sslOption, _reactor);
    if (!_bindAddress.getAddress().empty()) {
        EndpointType endpoint{boost::asio::ip::make_address(_bindAddress.getAddress()), _bindAddress.getPort()};
        _connection->getSocket().lowest_layer().open(endpoint.protocol());
        _connection->getSocket().lowest_layer().bind(endpoint);
    }
}

void SSLConnector::abortConnecting() {
    if (_connection) {
        _connection->getSocket().lowest_layer().close();
        _connection.reset();
    } else {
        _resolver.cancel();
    }
    _state = kDisconnecting;
}

NS_END