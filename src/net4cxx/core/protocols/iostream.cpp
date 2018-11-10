//
// Created by yuwenyong.vincent on 2018/10/13.
//


#include "net4cxx/core/protocols/iostream.h"


NS_BEGIN


constexpr size_t IOStream::DEFAULT_MAX_BUFFER_SIZE;

void IOStream::dataReceived(Byte *data, size_t length) {
    _readBuffer.ensureFreeSpace(length);
    _readBuffer.write(data, length);
    if (_readBuffer.getBufferSize() > _maxBufferSize) {
        NET4CXX_LOG_ERROR(gGenLog, "Reached maximum read buffer size");
        closeStream(std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(StreamBufferFullError,
                                                                   "Reached maximum read buffer size")));
    } else {
        try {
            readFromBuffer();
        } catch (...) {
            closeStream(std::current_exception());
        }
    }
}

void IOStream::connectionLost(std::exception_ptr reason) {
    if (_readUntilClose) {
        _readUntilClose = false;
        size_t numBytes = _readBuffer.getActiveSize();
        dataRead(_readBuffer.getReadPointer(), numBytes);
        _readBuffer.readCompleted(numBytes);
    }
    auto error = _error ? _error : reason;
    _error = nullptr;
    connectionClose(error);
}

void IOStream::connectionClose(std::exception_ptr reason) {

}

void IOStream::readUntilRegex(const std::string &regex, size_t maxBytes) {
    NET4CXX_ASSERT_MSG(!reading(), "Already reading");
    _readRegex = boost::regex(regex);
    _readMaxBytes = maxBytes;
    tryInlineRead();
}

void IOStream::readUntil(std::string delimiter, size_t maxBytes) {
    NET4CXX_ASSERT_MSG(!reading(), "Already reading");
    _readDelimiter = std::move(delimiter);
    _readMaxBytes = maxBytes;
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

void IOStream::readFromBuffer() {
    if (disconnected() && _readUntilClose) {
        _readUntilClose = false;
        size_t numBytes = _readBuffer.getActiveSize();
        dataRead(_readBuffer.getReadPointer(), numBytes);
        _readBuffer.readCompleted(numBytes);
    } else if (_readBytes != 0) {
        if (_readBuffer.getActiveSize() >= _readBytes) {
            size_t numBytes = _readBytes;
            _readBytes = 0;
            dataRead(_readBuffer.getReadPointer(), numBytes);
            _readBuffer.readCompleted(numBytes);
        }
    } else if (!_readDelimiter.empty()) {
        const char *loc = StrNStr((const char *)_readBuffer.getReadPointer(), _readBuffer.getActiveSize(),
                                  _readDelimiter.c_str());
        if (loc) {
            size_t readBytes = loc - (const char *)_readBuffer.getReadPointer() + _readDelimiter.size();
            checkMaxBytes(_readDelimiter, readBytes);
            _readDelimiter.clear();
            dataRead(_readBuffer.getReadPointer(), readBytes);
            _readBuffer.readCompleted(readBytes);
        } else {
            checkMaxBytes(_readDelimiter, _readBuffer.getActiveSize());
        }
    } else if (!_readRegex.empty()) {
        boost::cmatch m;
        if (boost::regex_search((const char *) _readBuffer.getReadPointer(),
                                (const char *) _readBuffer.getReadPointer() + _readBuffer.getActiveSize(), m,
                                _readRegex)) {
            auto readBytes = m.position((size_t) 0) + m.length();
            checkMaxBytes(_readRegex.str(), (size_t)readBytes);
            _readRegex = {};
            dataRead(_readBuffer.getReadPointer(), (size_t)readBytes);
            _readBuffer.readCompleted((size_t)readBytes);
        } else {
            checkMaxBytes(_readRegex.str(), _readBuffer.getActiveSize());
        }
    }
}

void IOStream::checkMaxBytes(const std::string &delimiter, size_t size) {
    if (_readMaxBytes != 0 && size > _readMaxBytes) {
        NET4CXX_THROW_EXCEPTION(UnsatisfiableReadError, "delimiter %s not found within %lu bytes", delimiter,
                                _readMaxBytes);
    }
}


NS_END