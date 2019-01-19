//
// Created by yuwenyong.vincent on 2018/10/13.
//


#include "net4cxx/core/protocols/iostream.h"


NS_BEGIN


constexpr size_t IOStream::DEFAULT_MAX_BUFFER_SIZE;

void IOStream::connectionMade() {
    auto producer = std::make_shared<WriteDoneTrigger>(getSelf<IOStream>());
    registerProducer(producer, false);
    onConnected();
}

void IOStream::dataReceived(Byte *data, size_t length) {
    _readBuffer.ensureFreeSpace(length);
    _readBuffer.write(data, length);
    try {
        if (_readBuffer.getBufferSize() > _maxBufferSize) {
            NET4CXX_LOG_ERROR(gGenLog, "Reached maximum read buffer size");
            NET4CXX_THROW_EXCEPTION(StreamBufferFullError, "Reached maximum read buffer size");
        } else {
            readFromBuffer();
        }
    } catch (...) {
        close(std::current_exception());
    }
}

void IOStream::connectionLost(std::exception_ptr reason) {
    if (_readUntilClose) {
        _readUntilClose = false;
        size_t numBytes = _readBuffer.getActiveSize();
        onDataRead(_readBuffer.getReadPointer(), numBytes);
        _readBuffer.readCompleted(numBytes);
    }
    auto error = _error ? _error : reason;
    _error = nullptr;
    onDisconnected(error);
}

void IOStream::onConnected() {

}

void IOStream::onWriteComplete() {

}

void IOStream::onDisconnected(std::exception_ptr reason) {

}

void IOStream::readUntilRegex(const std::string &regex, size_t maxBytes) {
    NET4CXX_ASSERT_MSG(!reading(), "Already reading");
    _readRegex = boost::regex(regex);
    if (maxBytes) {
        _readMaxBytes = maxBytes;
    }
    tryInlineRead();
}

void IOStream::readUntil(std::string delimiter, size_t maxBytes) {
    NET4CXX_ASSERT_MSG(!reading(), "Already reading");
    _readDelimiter = std::move(delimiter);
    if (maxBytes) {
        _readMaxBytes = maxBytes;
    }
    tryInlineRead();
}

void IOStream::readBytes(size_t numBytes) {
    NET4CXX_ASSERT_MSG(!reading(), "Already reading");
    _readBytes = numBytes;
    tryInlineRead();
}

void IOStream::readUntilClose() {
    NET4CXX_ASSERT_MSG(!reading(), "Already reading");
    _readUntilClose = true;
    tryInlineRead();
}

void IOStream::write(const Byte *data, size_t length, bool writeCallback) {
    if (closed()) {
        NET4CXX_THROW_EXCEPTION(StreamClosedError, "Already closed");
    }
    _writeCallback = writeCallback;
    Protocol::write(data, length);
}

void IOStream::close(std::exception_ptr error) {
    if (!closed()) {
        _error = error;
        loseConnection();
    }
}

void IOStream::tryInlineRead() {
    if (findReadPos()) {
        reactor()->addCallback([this, self=shared_from_this()]() {
            try {
                readFromBuffer();
            } catch (...) {
                close(std::current_exception());
            }
        });
    }
}

void IOStream::readFromBuffer() {
    if (disconnected() && _readUntilClose) {
        _readUntilClose = false;
        size_t numBytes = _readBuffer.getActiveSize();
        onDataRead(_readBuffer.getReadPointer(), numBytes);
        _readBuffer.readCompleted(numBytes);
    } else if (_readBytes) {
        if (_readBuffer.getActiveSize() >= *_readBytes) {
            size_t numBytes = *_readBytes;
            _readBytes = boost::none;
            onDataRead(_readBuffer.getReadPointer(), numBytes);
            _readBuffer.readCompleted(numBytes);
        }
    } else if (_readDelimiter) {
        const char *loc = StrNStr((const char *)_readBuffer.getReadPointer(), _readBuffer.getActiveSize(),
                                  _readDelimiter->c_str());
        if (loc) {
            size_t readBytes = (size_t)(loc - (const char *)_readBuffer.getReadPointer()) + _readDelimiter->size();
            checkMaxBytes(*_readDelimiter, readBytes);
            _readDelimiter = boost::none;
            onDataRead(_readBuffer.getReadPointer(), readBytes);
            _readBuffer.readCompleted(readBytes);
        } else {
            checkMaxBytes(*_readDelimiter, _readBuffer.getActiveSize());
        }
    } else if (_readRegex) {
        boost::cmatch m;
        if (boost::regex_search((const char *) _readBuffer.getReadPointer(),
                                (const char *) _readBuffer.getReadPointer() + _readBuffer.getActiveSize(), m,
                                *_readRegex)) {
            auto readBytes = m.position((size_t) 0) + m.length();
            checkMaxBytes(_readRegex->str(), (size_t)readBytes);
            _readRegex = boost::none;
            onDataRead(_readBuffer.getReadPointer(), (size_t)readBytes);
            _readBuffer.readCompleted((size_t)readBytes);
        } else {
            checkMaxBytes(_readRegex->str(), _readBuffer.getActiveSize());
        }
    }
}

bool IOStream::findReadPos() const {
    if (disconnected() && _readUntilClose) {
        return true;
    } else if (_readBytes) {
        return _readBuffer.getActiveSize() >= *_readBytes;
    } else if (_readDelimiter) {
        return StrNStr((const char *)_readBuffer.getReadPointer(), _readBuffer.getActiveSize(),
                       _readDelimiter->c_str()) != nullptr;
    } else if (_readRegex) {
        return boost::regex_search((const char *) _readBuffer.getReadPointer(),
                                   (const char *) _readBuffer.getReadPointer() + _readBuffer.getActiveSize(),
                                   *_readRegex);
    } else {
        return false;
    }
}

void IOStream::checkMaxBytes(const std::string &delimiter, size_t size) const {
    if (_readMaxBytes && size > *_readMaxBytes) {
        NET4CXX_THROW_EXCEPTION(UnsatisfiableReadError, "delimiter %s not found within %lu bytes", delimiter,
                                *_readMaxBytes);
    }
}


NS_END