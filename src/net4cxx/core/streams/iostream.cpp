//
// Created by yuwenyong.vincent on 2018/7/14.
//

#include "net4cxx/core/streams/iostream.h"


NS_BEGIN

BaseIOStream::BaseIOStream(SocketType &&socket, Reactor *reactor, size_t maxBufferSize, size_t readChunkSize)
        : _socket(std::move(socket))
        , _reactor(reactor ? reactor : Reactor::current())
        , _maxBufferSize(maxBufferSize ? maxBufferSize : DEFAULT_MAX_BUFFER_SIZE)
        , _readBuffer(readChunkSize) {

}

void BaseIOStream::clearCallbacks() {
    _readCallback = nullptr;
    _streamingCallback = nullptr;
    _writeCallback = nullptr;
    _closeCallback = nullptr;
    _connectCallback = nullptr;
}

void BaseIOStream::connect(ResolverResultsType addresses, ConnectCallbackType callback) {
    _connecting = true;
    _connectCallback = std::move(callback);
    realConnect(std::move(addresses));
}

void BaseIOStream::connect(const std::string &address, unsigned short port, ConnectCallbackType callback) {
    _connecting = true;
    _connectCallback = std::move(callback);
    realConnect(address, port);
}

void BaseIOStream::readUntilRegex(const std::string &regex, ReadCallbackType callback) {
    setReadCallback(std::move(callback));
    _readRegex = boost::regex(regex);
    tryInlineRead();
}

void BaseIOStream::readUntil(std::string delimiter, ReadCallbackType callback) {
    setReadCallback(std::move(callback));
    _readDelimiter = std::move(delimiter);
    tryInlineRead();
}

void BaseIOStream::readBytes(size_t numBytes, ReadCallbackType callback, StreamingCallbackType streamingCallback) {
    setReadCallback(std::move(callback));
    _readBytes = numBytes;
    _streamingCallback = std::move(streamingCallback);
    tryInlineRead();
}

void BaseIOStream::readUntilClose(ReadCallbackType callback, StreamingCallbackType streamingCallback) {
    setReadCallback(std::move(callback));
    _streamingCallback = std::move(streamingCallback);
    if (closed()) {
        if (_streamingCallback) {
            ByteArray data(_readBuffer.getReadPointer(), _readBuffer.getReadPointer() + _readBuffer.getActiveSize());
            _readBuffer.readCompleted(_readBuffer.getActiveSize());
            runCallback([streamingCallback=_streamingCallback, data=std::move(data)](){
                streamingCallback(std::move(data));
            });
        }
        callback = std::move(_readCallback);
        ByteArray data;
        if (_readBuffer.getActiveSize()) {
            data.assign(_readBuffer.getReadPointer(), _readBuffer.getReadPointer() + _readBuffer.getActiveSize());
            _readBuffer.readCompleted(_readBuffer.getActiveSize());
        }
        runCallback([callback=std::move(callback), data=std::move(data)](){
            callback(std::move(data));
        });
        _streamingCallback = nullptr;
        _readCallback = nullptr;
        return;
    }
    _readUntilClose = true;
    tryInlineRead();
}

void BaseIOStream::write(const Byte *data, size_t length,  WriteCallbackType callback) {
    checkClosed();
    if (length) {
        constexpr size_t WRITE_BUFFER_CHUNK_SIZE = 128 * 1024;
        if (length > WRITE_BUFFER_CHUNK_SIZE) {
            size_t chunkSize;
            for (size_t i = 0; i < length; i += chunkSize) {
                chunkSize = std::min(WRITE_BUFFER_CHUNK_SIZE, length - i);
                MessageBuffer packet(chunkSize);
                packet.write(data + i, chunkSize);
                _writeQueue.push_back(std::move(packet));
            }
        } else {
            MessageBuffer packet(length);
            packet.write(data, length);
            _writeQueue.push_back(std::move(packet));
        }
    }
    _writeCallback = callback;
    if (!_connecting) {
        if (!_writeQueue.empty()) {
            if ((_state & S_WRITE) == S_NONE) {
                writeToSocket();
            }
        } else {
            runCallback(std::move(_writeCallback));
            _writeCallback = nullptr;
        }
    }
}

void BaseIOStream::setCloseCallback(CloseCallbackType callback) {
    _closeCallback = std::move(callback);
}

void BaseIOStream::close(std::exception_ptr error) {
    if (!closed()) {
        if (error) {
            _error = error;
        }
        if (_readUntilClose) {
            if (_streamingCallback && _readBuffer.getActiveSize()) {
                ByteArray data(_readBuffer.getReadPointer(),
                               _readBuffer.getReadPointer() + _readBuffer.getActiveSize());
                runCallback([streamingCallback=_streamingCallback, data=std::move(data)](){
                    streamingCallback(std::move(data));
                });
                _readBuffer.readCompleted(_readBuffer.getActiveSize());
            }
            ReadCallbackType callback(std::move(_readCallback));
            _readCallback = nullptr;
            _readUntilClose = false;
            ByteArray data(_readBuffer.getReadPointer(), _readBuffer.getReadPointer() + _readBuffer.getActiveSize());
            _readBuffer.readCompleted(_readBuffer.getActiveSize());
            runCallback([callback=std::move(callback), data=std::move(data)](){
                callback(std::move(data));
            });
        }
        _closing = true;
        closeSocket();
    }
    maybeRunCloseCallback();
}

void BaseIOStream::onConnect(const boost::system::error_code &ec) {
    _state &= ~S_WRITE;
    if (ec) {
        _connectCallback = nullptr;
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_WARN(gGenLog, "Connect error %d :%s", ec.value(), ec.message());
            close(std::make_exception_ptr(boost::system::system_error(ec)));
        } else {
            maybeRunCloseCallback();
        }
        return;
    } else {
        if (_connectCallback) {
            ConnectCallbackType callback(std::move(_connectCallback));
            _connectCallback = nullptr;
            runCallback(std::move(callback));
        }
    }
    _socket.non_blocking(true);
    _connecting = false;
    if (!_writeQueue.empty()) {
        writeToSocket();
    } else if (_writeCallback) {
        runCallback(std::move(_writeCallback));
        _writeCallback = nullptr;
    } else {
        maybeAddErrorListener();
    }
}

void BaseIOStream::onRead(const boost::system::error_code &ec, size_t transferredBytes) {
    _state &= ~S_READ;
    try {
        if (readToBuffer(ec, transferredBytes) == 0) {
            return;
        }
    } catch (std::exception &e) {
        close();
        return;
    }
    if (readFromBuffer()) {
        return;
    }
    if (closed()) {
        return;
    }
    if (reading()) {
        readFromSocket();
    } else {
        maybeAddErrorListener();
    }
}

void BaseIOStream::onWrite(const boost::system::error_code &ec, size_t transferredBytes) {
    _state &= ~S_WRITE;
    if (ec) {
        _writeCallback = nullptr;
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_WARN(gGenLog, "Write error %d :%s", ec.value(), ec.message());
            close(std::make_exception_ptr(boost::system::system_error(ec)));
        } else {
            maybeRunCloseCallback();
        }
        return;
    }
    if (transferredBytes > 0) {
        _writeQueue.front().readCompleted(transferredBytes);
        if (!_writeQueue.front().getActiveSize()) {
            _writeQueue.pop_front();
        }
    }
    if (_writeQueue.empty() && _writeCallback) {
        WriteCallbackType callback;
        callback.swap(_writeCallback);
        runCallback(std::move(callback));
        return;
    }
    if (closed()) {
        return;
    }
    if (writing()) {
        writeToSocket();
    } else {
        maybeAddErrorListener();
    }
}

void BaseIOStream::onClose(const boost::system::error_code &ec) {
    _closing = false;
    _closed = true;
    if (ec) {
        if (!_error) {
            _error = std::make_exception_ptr(boost::system::system_error(ec));
        }
        NET4CXX_LOG_WARN(gGenLog, "Close error %d :%s", ec.value(), ec.message());
    }
    maybeRunCloseCallback();
}

void BaseIOStream::maybeRunCloseCallback() {
    if (_closed && _pendingCallbacks == 0) {
        if (_closeCallback) {
            CloseCallbackType callback(std::move(_closeCallback));
            _closeCallback = nullptr;
            runCallback(std::move(callback));
        }
        clearCallbacks();
        _writeQueue.clear();
    }
}

void BaseIOStream::runCallback(CallbackType callback) {
    ++_pendingCallbacks;
    Wrapper1 op(shared_from_this(), [this, callback=std::move(callback)](){
        --_pendingCallbacks;
        try {
            callback();
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gAppLog, "Uncaught exception (%s), closing connection.", e.what());
            close(std::current_exception());
            throw;
        } catch (...) {
            NET4CXX_LOG_ERROR(gAppLog, "Unknown exception, closing connection.");
            close(std::current_exception());
        }
        maybeAddErrorListener();
    });
    _reactor->addCallback(std::move(op));
}

size_t BaseIOStream::readToBuffer(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            if (ec != boost::asio::error::eof) {
                NET4CXX_LOG_WARN(gGenLog, "Read error %d :%s", ec.value(), ec.message());
            }
            close(std::make_exception_ptr(boost::system::system_error(ec)));
        } else {
            maybeRunCloseCallback();
        }
        return 0;
    }
    _readBuffer.writeCompleted(transferredBytes);
    if (_readBuffer.getBufferSize() > _maxBufferSize) {
        NET4CXX_LOG_ERROR(gGenLog, "Reached maximum read buffer size");
        close();
        NET4CXX_THROW_EXCEPTION(IOError, "Reached maximum read buffer size");
    }
    return transferredBytes;
}

bool BaseIOStream::readFromBuffer() {
    if (_streamingCallback && _readBuffer.getActiveSize() > 0) {
        size_t bytesToConsume = _readBuffer.getActiveSize();
        if (_readBytes) {
            bytesToConsume = std::min(_readBytes.get(), bytesToConsume);
            _readBytes.get() -= bytesToConsume;
        }
        ByteArray data(_readBuffer.getReadPointer(), _readBuffer.getReadPointer() + bytesToConsume);
        _readBuffer.readCompleted(bytesToConsume);
        runCallback([streamingCallback=_streamingCallback, data=std::move(data)](){
            streamingCallback(std::move(data));
        });
    }
    if (_readBytes && _readBuffer.getActiveSize() >= _readBytes.get()) {
        size_t numBytes = _readBytes.get();
        ReadCallbackType callback(std::move(_readCallback));
        _readCallback = nullptr;
        _streamingCallback = nullptr;
        _readBytes = boost::none;
        ByteArray data;
        if (numBytes > 0) {
            data.assign(_readBuffer.getReadPointer(), _readBuffer.getReadPointer() + numBytes);
            _readBuffer.readCompleted(numBytes);
        }
        runCallback([callback=std::move(callback), data=std::move(data)](){
            callback(std::move(data));
        });
        return true;
    } else if (_readDelimiter) {
        const char *loc = StrNStr((const char *)_readBuffer.getReadPointer(), _readBuffer.getActiveSize(),
                                  _readDelimiter->c_str());
        if (loc) {
            size_t readBytes = loc - (const char *)_readBuffer.getReadPointer() + _readDelimiter->size();
            ReadCallbackType callback(std::move(_readCallback));
            _readCallback = nullptr;
            _streamingCallback = nullptr;
            _readDelimiter = boost::none;
            ByteArray data(_readBuffer.getReadPointer(), _readBuffer.getReadPointer() + readBytes);
            _readBuffer.readCompleted(readBytes);
            runCallback([callback=std::move(callback), data=std::move(data)](){
                callback(std::move(data));
            });
            return true;
        }
    } else if (_readRegex) {
        boost::cmatch m;
        if (boost::regex_search((const char *) _readBuffer.getReadPointer(),
                                (const char *) _readBuffer.getReadPointer() + _readBuffer.getActiveSize(), m,
                                _readRegex.get())) {
            auto readBytes = m.position((size_t) 0) + m.length();
            ReadCallbackType callback(std::move(_readCallback));
            _readCallback = nullptr;
            _streamingCallback = nullptr;
            _readRegex = boost::none;
            ByteArray data(_readBuffer.getReadPointer(), _readBuffer.getReadPointer() + readBytes);
            _readBuffer.readCompleted((size_t) readBytes);
            runCallback([callback = std::move(callback), data = std::move(data)]() {
                callback(std::move(data));
            });
            return true;
        }
    }
    return false;
}

void BaseIOStream::maybeAddErrorListener() {
    if (_state == S_NONE && _pendingCallbacks == 0) {
        if (closed()) {
            maybeRunCloseCallback();
        } else {
            readFromSocket();
        }
    }
}


void IOStream::realConnect(ResolverResultsType addresses) {
//    ResolverType resolver(_reactor->getIOContext());
//    boost::system::error_code ec;
//    auto results = resolver.resolve(address, std::to_string(port), ec);
//    if (ec) {
//        onConnect(ec);
//        return;
//    }
    Wrapper4 op(shared_from_this(), [this](const boost::system::error_code &ec, const EndpointType &ep) {
        onConnect(ec);
    });
    boost::asio::async_connect(_socket, addresses, std::move(op));
    _state |= S_WRITE;
}

void IOStream::realConnect(const std::string &address, unsigned short port) {
    EndpointType endpoint(boost::asio::ip::make_address(address), port);
    Wrapper2 op(shared_from_this(), [this](const boost::system::error_code &ec) {
        onConnect(ec);
    });
    _socket.async_connect(endpoint, std::move(op));
    _state |= S_WRITE;
}

void IOStream::readFromSocket() {
    _readBuffer.normalize();
    _readBuffer.ensureFreeSpace();
    Wrapper3 op(shared_from_this(), [this](const boost::system::error_code &ec, size_t transferredBytes) {
        onRead(ec, transferredBytes);
    });
    _socket.async_read_some(boost::asio::buffer(_readBuffer.getWritePointer(), _readBuffer.getRemainingSpace()),
                            std::move(op));
    _state |= S_READ;
}


void IOStream::writeToSocket() {
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
            }
            onWrite(ec, 0);
            return;
        } else if (bytesSent == 0){
            close();
            return;
        } else if (bytesSent < bytesToSend) {
            buffer.readCompleted(bytesSent);
            break;
        }
        _writeQueue.pop_front();
        if (_writeQueue.empty()) {
            onWrite(ec, 0);
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
    Wrapper3 op(shared_from_this(), [this](const boost::system::error_code &ec, size_t transferredBytes) {
        onWrite(ec, transferredBytes);
    });
    _socket.async_write_some(boost::asio::buffer(buffer.getReadPointer(), buffer.getActiveSize()), std::move(op));
    _state |= S_WRITE;
}

void IOStream::closeSocket() {
    boost::system::error_code ec;
    _socket.close(ec);
    onClose(ec);
}



void SSLIOStream::clearCallbacks() {
    BaseIOStream::clearCallbacks();
    _sslConnectCallback = nullptr;
}

void SSLIOStream::realConnect(ResolverResultsType addresses) {
//    ResolverType resolver(_reactor->getIOContext());
//    boost::system::error_code ec;
//    auto results = resolver.resolve(address, std::to_string(port), ec);
//    if (ec) {
//        onConnect(ec);
//        return;
//    }
    _sslConnectCallback = std::move(_connectCallback);
    _connectCallback = nullptr;
    Wrapper4 op(shared_from_this(), [this](const boost::system::error_code &ec, const EndpointType &ep) {
        onConnect(ec);
    });
    boost::asio::async_connect(_sslSocket.lowest_layer(), addresses, std::move(op));
    _state |= S_WRITE;
}

void SSLIOStream::realConnect(const std::string &address, unsigned short port) {
    EndpointType endpoint(boost::asio::ip::make_address(address), port);
    _sslConnectCallback = std::move(_connectCallback);
    _connectCallback = nullptr;
    Wrapper2 op(shared_from_this(), [this](const boost::system::error_code &ec) {
        onConnect(ec);
    });
    _sslSocket.lowest_layer().async_connect(endpoint, std::move(op));
}

void SSLIOStream::readFromSocket() {
    if (_sslAccepted) {
        doRead();
    } else {
        doHandshake();
    }
}

void SSLIOStream::writeToSocket() {
    if (_sslAccepted) {
        doWrite();
    } else {
        doHandshake();
    }
}

void SSLIOStream::closeSocket() {
    if (_sslAccepted && ((_state & S_WRITE) == S_NONE)) {
        Wrapper2 op(shared_from_this(), [this](const boost::system::error_code &ec) {
            onShutdown(ec);
        });
        _sslSocket.async_shutdown(std::move(op));
        _state |= S_WRITE;
    } else {
        doClose();
    }
}

void SSLIOStream::doHandshake() {
    if (!_sslAccepted && !_sslAccepting) {
        Wrapper2 op(shared_from_this(), [this](const boost::system::error_code &ec) {
            onHandshake(ec);
        });
        if (_sslOption->isServerSide()) {
            _sslSocket.async_handshake(boost::asio::ssl::stream_base::server, std::move(op));

        } else {
            _sslSocket.async_handshake(boost::asio::ssl::stream_base::client, std::move(op));
        }
        _sslAccepting = true;
        _state |= S_WRITE;
    }
}

void SSLIOStream::doRead() {
    _readBuffer.normalize();
    _readBuffer.ensureFreeSpace();
    Wrapper3 op(shared_from_this(), [this](const boost::system::error_code &ec, size_t transferredBytes) {
        onRead(ec, transferredBytes);
    });
    _sslSocket.async_read_some(boost::asio::buffer(_readBuffer.getWritePointer(), _readBuffer.getRemainingSpace()),
                               std::move(op));
    _state |= S_READ;
}

void SSLIOStream::doWrite() {
    Wrapper3 op(shared_from_this(), [this](const boost::system::error_code &ec, size_t transferredBytes) {
        onWrite(ec, transferredBytes);
    });
    MessageBuffer& buffer = _writeQueue.front();
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
    _sslSocket.async_write_some(boost::asio::buffer(buffer.getReadPointer(), buffer.getActiveSize()),
                                std::move(op));
    _state |= S_WRITE;
}

void SSLIOStream::doClose() {
    boost::system::error_code ec;
    _socket.close(ec);
    onClose(ec);
}

void SSLIOStream::onHandshake(const boost::system::error_code &ec) {
    _state &= ~S_WRITE;
    _sslAccepting = false;
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_LOG_WARN(gGenLog, "SSL error %d :%s, closing connection.", ec.value(), ec.message());
            close(std::make_exception_ptr(boost::system::system_error(ec)));
        }
    } else {
        _sslAccepted = true;
        if (_sslConnectCallback) {
            ConnectCallbackType callback(std::move(_sslConnectCallback));
            _sslConnectCallback = nullptr;
            runCallback(std::move(callback));
        }
        if (writing()) {
            doWrite();
        }
        if (reading()) {
            doRead();
        }
        maybeAddErrorListener();
    }
}

void SSLIOStream::onShutdown(const boost::system::error_code &ec) {
    _state &= ~S_WRITE;
    if (ec.category() == boost::asio::error::get_ssl_category()) {
        NET4CXX_LOG_WARN(gGenLog, "SSL shutdown error %d :%s", ec.value(), ec.message());
    }
    doClose();
}

NS_END