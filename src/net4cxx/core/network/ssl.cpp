//
// Created by yuwenyong on 17-10-10.
//

#include "net4cxx/core/network/ssl.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN


SSLConnection::SSLConnection(const ProtocolPtr &protocol, SSLOptionPtr sslOption, Reactor *reactor)
        : Connection(protocol, reactor)
        , _sslOption(std::move(sslOption))
        , _socket(reactor->getService(), _sslOption->context()) {

}

void SSLConnection::write(const Byte *data, size_t length) {
    if (_disconnecting || _disconnected || !_connected) {
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
    _error = NET4CXX_EXCEPTION_PTR(ConnectionDone, "");
    _disconnecting = true;
    doClose();
}

void SSLConnection::abortConnection() {
    if (_disconnecting || _disconnected || !_connected) {
        return;
    }
    _error = NET4CXX_EXCEPTION_PTR(ConnectionAbort, "");
    _disconnecting = true;
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
        _reactor->addCallback([this, self=shared_from_this()]() {
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
        _reactor->addCallback([this, self=shared_from_this()]() {
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
    if (_socket.lowest_layer().is_open()) {
        _socket.lowest_layer().close();
    }
    connectionLost(_error);
}

void SSLConnection::doHandshake() {
    auto protocol = _protocol.lock();
    BOOST_ASSERT(protocol);
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
            NET4CXX_ERROR(gGenLog, "Handshake error %d :%s", ec.value(), ec.message().c_str());
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
    BOOST_ASSERT(protocol);
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
            NET4CXX_ERROR(gGenLog, "Read error %d :%s", ec.value(), ec.message().c_str());
        }
        if (!_disconnected) {
            if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof &&
                (ec.category() != boost::asio::error::get_ssl_category() ||
                 ERR_GET_REASON(ec.value()) != SSL_R_SHORT_READ)) {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
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
    BOOST_ASSERT(protocol);
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
            NET4CXX_ERROR(gGenLog, "Write error %d :%s", ec.value(), ec.message().c_str());
        }
        if (!_disconnected) {
            if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof &&
                (ec.category() != boost::asio::error::get_ssl_category() ||
                 ERR_GET_REASON(ec.value()) != SSL_R_SHORT_READ)) {
                _error = std::make_exception_ptr(boost::system::system_error(ec));
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
        if (_disconnecting) {
            startShutdown();
        }
    }
}

void SSLConnection::doShutdown() {
    auto protocol = _protocol.lock();
    BOOST_ASSERT(protocol);
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
            NET4CXX_ERROR(gGenLog, "Read error %d :%s", ec.value(), ec.message().c_str());
            _error = std::make_exception_ptr(boost::system::system_error(ec));
        }
    }
    BOOST_ASSERT(!_disconnected);
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

NS_END