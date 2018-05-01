//
// Created by yuwenyong on 17-11-21.
//

#include "net4cxx/core/network/unix.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

NS_BEGIN

UNIXConnection::UNIXConnection(const ProtocolPtr &protocol, Reactor *reactor)
        : Connection(protocol, reactor)
        , _socket(reactor->getService()) {

}

void UNIXConnection::write(const Byte *data, size_t length) {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    MessageBuffer packet(length);
    packet.write(data, length);
    _writeQueue.emplace_back(std::move(packet));
    startWriting();
}

void UNIXConnection::loseConnection() {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    _error = NET4CXX_EXCEPTION_PTR(ConnectionDone, "");
    _disconnecting = true;
    doClose();
}

void UNIXConnection::abortConnection() {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    _error = NET4CXX_EXCEPTION_PTR(ConnectionAbort, "");
    _disconnecting = true;
    _aborting = true;
    doAbort();
}

void UNIXConnection::doClose() {
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

void UNIXConnection::doAbort() {
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

void UNIXConnection::closeSocket() {
    _connected = false;
    _disconnected = true;
    _disconnecting = false;
    _aborting = false;
    if (_socket.is_open()) {
        _socket.close();
    }
    connectionLost(_error);
}

void UNIXConnection::doRead() {
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

void UNIXConnection::handleRead(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {
            NET4CXX_LOG_ERROR(gGenLog, "Read error %d :%s", ec.value(), ec.message().c_str());
        }
        if (!_disconnected) {
            if (ec == boost::asio::error::operation_aborted) {
                NET4CXX_ASSERT(_error);
            } else if (ec == boost::asio::error::eof) {
                _error = NET4CXX_EXCEPTION_PTR(ConnectionDone, "");
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

void UNIXConnection::doWrite() {
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
    MessageBuffer &buffer = _writeQueue.front();
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    _writing = true;
    _socket.async_write_some(boost::asio::buffer(buffer.getReadPointer(), buffer.getActiveSize()),
                             [protocol, self = shared_from_this()](const boost::system::error_code &ec,
                                                                   size_t transferredBytes) {
                                 self->cbWrite(ec, transferredBytes);
                             });
}

void UNIXConnection::handleWrite(const boost::system::error_code &ec, size_t transferredBytes) {
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


void UNIXServerConnection::cbAccept(const ProtocolPtr &protocol) {
    _protocol = protocol;
    _connected = true;
    _socket.non_blocking(true);
    protocol->makeConnection(shared_from_this());
    if (!_disconnecting) {
        startReading();
    }
}


void UNIXClientConnection::cbConnect(const ProtocolPtr &protocol, std::shared_ptr<UNIXConnector> connector) {
    _protocol = protocol;
    _connector = std::move(connector);
    _connected = true;
    _socket.non_blocking(true);
    protocol->makeConnection(shared_from_this());
    if (!_disconnecting) {
        startReading();
    }
}

void UNIXClientConnection::closeSocket() {
    UNIXConnection::closeSocket();
    _connector->connectionLost(_error);
}


UNIXListener::UNIXListener(std::string path, std::shared_ptr<Factory> factory, Reactor *reactor)
        : Listener(reactor)
        , _path(std::move(path))
        , _factory(std::move(factory))
        , _acceptor(reactor->getService()) {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(NET4CXX_UNIXListener_COUNT);
#endif
}

void UNIXListener::startListening() {
    EndpointType endpoint(_path);
    ::unlink(_path.c_str());
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(boost::asio::socket_base::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen();
    NET4CXX_LOG_INFO(gGenLog, "UNIXListener starting on %s", _path.c_str());
    _factory->doStart();
    _connected = true;
    doAccept();
}

void UNIXListener::stopListening() {
    if (_connected) {
        _connected = false;
        _acceptor.close();
        _factory->doStop();
        NET4CXX_LOG_INFO(gGenLog, "UNIXListener closed on %s", _path.c_str());
    }
}

void UNIXListener::cbAccept(const boost::system::error_code &ec) {
    handleAccept(ec);
    if (!_connected) {
        return;
    }
    doAccept();
}

void UNIXListener::handleAccept(const boost::system::error_code &ec) {
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


UNIXConnector::UNIXConnector(std::string path, std::shared_ptr<ClientFactory> factory, double timeout,
                             Reactor *reactor)
        : Connector(reactor)
        , _path(std::move(path))
        , _factory(std::move(factory))
        , _timeout(timeout) {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(NET4CXX_UNIXConnector_COUNT);
#endif
}

void UNIXConnector::startConnecting() {
    if (_state != kDisconnected) {
        NET4CXX_THROW_EXCEPTION(Exception, "Can't connect in this state");
    }
    _state = kConnecting;
    if (!_factoryStarted) {
        _factory->doStart();
        _factoryStarted = true;
    }
    doConnect();
    if (_timeout != 0.0) {
        _timeoutId = _reactor->callLater(_timeout, [this, self=shared_from_this()]() {
            cbTimeout();
        });
    }
    _factory->startedConnecting(shared_from_this());
}

void UNIXConnector::stopConnecting() {
    if (_state != kConnecting) {
        NET4CXX_THROW_EXCEPTION(NotConnectingError, "We're not trying to connect");
    }
    _error = NET4CXX_EXCEPTION_PTR(UserAbort, "");
    NET4CXX_ASSERT(_connection);
    _connection->getSocket().close();
    _connection.reset();
    _state = kDisconnected;
}

void UNIXConnector::connectionFailed(std::exception_ptr reason) {
    if (reason) {
        _error = std::move(reason);
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

void UNIXConnector::connectionLost(std::exception_ptr reason) {
    if (reason) {
        _error = std::move(reason);
    }
    _state = kDisconnected;
    _factory->clientConnectionLost(shared_from_this(), _error);
    if (_state == kDisconnected) {
        _factory->doStop();
        _factoryStarted = false;
    }
}

ProtocolPtr UNIXConnector::buildProtocol(const Address &address) {
    _state = kConnected;
    cancelTimeout();
    return _factory->buildProtocol(address);
}

void UNIXConnector::doConnect() {
    makeTransport();
    EndpointType endpoint{_path};
    _connection->getSocket().async_connect(endpoint, [this, self=shared_from_this(), connection=_connection](
            const boost::system::error_code &ec) {
        cbConnect(ec);
    });
}

void UNIXConnector::handleConnect(const boost::system::error_code &ec) {
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

void UNIXConnector::handleTimeout() {
    NET4CXX_LOG_ERROR(gGenLog, "Connect error : Timeout");
    _error = NET4CXX_EXCEPTION_PTR(TimeoutError, "");
    connectionFailed();
}

void UNIXConnector::makeTransport() {
    _connection = std::make_shared<UNIXClientConnection>(_reactor);
}


UNIXDatagramConnection::UNIXDatagramConnection(std::string path, const DatagramProtocolPtr &protocol,
                                               size_t maxPacketSize, Reactor *reactor)
        : DatagramConnection({std::move(path)}, protocol, maxPacketSize, reactor)
        , _socket(reactor->getService(), SocketType::protocol_type()) {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(NET4CXX_UNIXDatagramConnection_COUNT);
#endif
}

UNIXDatagramConnection::UNIXDatagramConnection(std::string path, const DatagramProtocolPtr &protocol,
                                               size_t maxPacketSize, std::string bindPath, Reactor *reactor)
        : DatagramConnection({std::move(path)}, protocol, maxPacketSize, {std::move(bindPath)}, reactor)
        , _socket(reactor->getService(), SocketType::protocol_type()) {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(NET4CXX_UNIXDatagramConnection_COUNT);
#endif
}

void UNIXDatagramConnection::write(const Byte *datagram, size_t length, const Address &address) {
    try {
        if (_connectedAddress) {
            NET4CXX_ASSERT(!address || address == _connectedAddress);
            _socket.send(boost::asio::buffer(datagram, length));
        } else {
            NET4CXX_ASSERT(address);
            EndpointType receiver(address.getAddress());
            _socket.send_to(boost::asio::buffer(datagram, length), receiver);
        }
    } catch (boost::system::system_error &e) {
        NET4CXX_LOG_INFO(gGenLog, "Write error %d: %s", e.code().value(), e.code().message().c_str());
        if (_connectedAddress) {
            connectionRefused();
        }
        throw;
    }
}

void UNIXDatagramConnection::connect(const Address &address) {
    if (_connectedAddress) {
        NET4CXX_THROW_EXCEPTION(AlreadyConnected, "Reconnecting is not currently supported");
    }
    _connectedAddress = address;
    connectSocket();
}

void UNIXDatagramConnection::loseConnection() {
    if (_reading && _socket.is_open()) {
        _socket.close();
    }
}

bool UNIXDatagramConnection::getBroadcastAllowed() const {
    boost::asio::socket_base::broadcast option;
    _socket.get_option(option);
    return option.value();
}

void UNIXDatagramConnection::setBroadcastAllowed(bool enabled) {
    boost::asio::socket_base::broadcast option(enabled);
    _socket.set_option(option);
}

std::string UNIXDatagramConnection::getLocalAddress() const {
    auto endpoint = _socket.remote_endpoint();
    return endpoint.path();
}

unsigned short UNIXDatagramConnection::getLocalPort() const {
    return 0;
}

std::string UNIXDatagramConnection::getRemoteAddress() const {
    return _connectedAddress.getAddress();
}

unsigned short UNIXDatagramConnection::getRemotePort() const {
    return 0;
}

void UNIXDatagramConnection::startListening() {
    try {
        if (_bindAddress) {
            bindSocket();
        }
        if (_connectedAddress) {
            connectSocket();
        }
        connectToProtocol();
    } catch (...) {
        connectionFailed(std::current_exception());
    }
}

void UNIXDatagramConnection::connectToProtocol() {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    protocol->makeConnection(shared_from_this());
    startReading();
}

void UNIXDatagramConnection::doRead() {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    _reading = true;
    _socket.async_receive_from(boost::asio::buffer(_readBuffer.data(), _readBuffer.size()), _sender,
                               [protocol, self = shared_from_this()](const boost::system::error_code &ec,
                                                                     size_t transferredBytes) {
                                   self->cbRead(ec, transferredBytes);
                               });
}

void UNIXDatagramConnection::handleRead(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_ERROR(gGenLog, "Read error %d :%s", ec.value(), ec.message().c_str());
            if (_connectedAddress) {
                connectionRefused();
            }
        } else {
            connectionLost();
            _reading = false;
        }
    } else {
        Address sender(_sender.path());
        datagramReceived(_readBuffer.data(), transferredBytes, std::move(sender));
    }
}

void UNIXDatagramConnection::bindSocket() {
    try {
        EndpointType endpoint{_bindAddress.getAddress()};
        ::unlink(_bindAddress.getAddress().c_str());
//        _socket.open(endpoint.protocol());
        _socket.bind(endpoint);
        NET4CXX_LOG_INFO(gGenLog, "UNIXDatagramConnection starting on %s", _bindAddress.getAddress().c_str());
    } catch (boost::system::system_error &e) {
        NET4CXX_LOG_ERROR(gGenLog, "Bind error %d: %s", e.code().value(), e.code().message().c_str());
        throw;
    }
}

void UNIXDatagramConnection::connectSocket() {
    try {
        EndpointType endpoint{_connectedAddress.getAddress()};
        _socket.connect(endpoint);
    } catch (boost::system::system_error &e) {
        NET4CXX_LOG_ERROR(gGenLog, "Connect error %d: %s", e.code().value(), e.code().message().c_str());
        throw;
    }
}

NS_END

#endif //BOOST_ASIO_HAS_LOCAL_SOCKETS