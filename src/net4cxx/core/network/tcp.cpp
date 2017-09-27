//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/tcp.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"

NS_BEGIN


TCPConnection::TCPConnection(const std::weak_ptr<Protocol> &protocol, Reactor *reactor)
        : Connection(protocol, reactor)
        , _socket(reactor->createSocket()) {

}

const char* TCPConnection::logPrefix() const {
    return "TCPConnection";
}

void TCPConnection::write(const Byte *data, size_t length) {
    if (_disconnecting || _disconnected || !_connected) {
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
    _disconnecting = true;
    if (!_writing) {
        _socket.close();
    }
}

void TCPConnection::abortConnection() {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    _disconnecting = true;
    _socket.close();
}

void TCPConnection::doRead() {
    auto protocol = _protocol.lock();
    BOOST_ASSERT(protocol);
    _readBuffer.normalize();
    _readBuffer.ensureFreeSpace();
    _reading = true;
    _socket.async_read_some(boost::asio::buffer(_readBuffer.getWritePointer(), _readBuffer.getRemainingSpace()),
                            [this, protocol](const boost::system::error_code &ec, size_t transferredBytes) {
                                cbRead(ec, transferredBytes);
                            });
}

void TCPConnection::handleRead(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {

        }
        if (!_disconnected) {
            _connected = false;
            _disconnected = true;
            _disconnecting = false;
            if (_socket.is_open()) {
                _socket.close();
            }
            if (ec != boost::asio::error::operation_aborted) {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
            }
            connectionLost(_error);
        }
    } else {
        if (_disconnecting) {
            return;
        }
        _readBuffer.writeCompleted(transferredBytes);
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

            }
            _error = std::make_exception_ptr(boost::system::system_error(ec));
            _disconnecting = true;
            _socket.close();
            return;
        } else if (bytesSent == 0){
            _disconnecting = true;
            _socket.close();
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
    BOOST_ASSERT(protocol);
    _writing = true;
    _socket.async_write_some(boost::asio::buffer(buffer.getReadPointer(), buffer.getActiveSize()),
                             [this, protocol](const boost::system::error_code &ec, size_t transferredBytes) {
                                 cbWrite(ec, transferredBytes);
                             });
}

void TCPConnection::handleWrite(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {

        }
        if (!_disconnected) {
            _connected = false;
            _disconnected = true;
            _disconnecting = false;
            if (_socket.is_open()) {
                _socket.close();
            }
            if (ec != boost::asio::error::operation_aborted) {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
            }
            connectionLost(_error);
        }
    } else {
        if (_disconnecting) {
            _connected = false;
            _disconnected = true;
            _disconnecting = false;
            _socket.close();
            connectionLost(_error);
            return;
        }
        if (transferredBytes > 0) {
            _writeQueue.front().readCompleted(transferredBytes);
            if (!_writeQueue.front().getActiveSize()) {
                _writeQueue.pop_front();
            }
        }
    }
}


TCPServerConnection::TCPServerConnection(unsigned int sessionno, Reactor *reactor)
        : TCPConnection({}, reactor)
        , _sessionno(sessionno) {
    _logstr = StrUtil::format("TCPServerConnection, %u", sessionno);
}

const char* TCPServerConnection::logPrefix() const {
    return _logstr.c_str();
}

void TCPServerConnection::cbAccept(const std::weak_ptr<Protocol> &protocol) {
    _protocol = protocol;
    _connected = true;
    _socket.non_blocking(true);
    startReading();
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
            _connection->cbAccept(protocol);
            protocol->makeConnection(_connection);
        }
        _connection.reset();
    }
}

NS_END
