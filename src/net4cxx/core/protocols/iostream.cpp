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
        loseConnection();
    } else {
        readFromBuffer();
    }
}

void IOStream::connectionLost(std::exception_ptr reason) {
    if (_readUntilClose) {
        _readUntilClose = false;
        size_t numBytes = _readBuffer.getActiveSize();
        dataRead(_readBuffer.getReadPointer(), numBytes);
        _readBuffer.readCompleted(numBytes);
    }
    connectionClose(reason);
}

void IOStream::connectionClose(std::exception_ptr reason) {

}

void IOStream::readUntilRegex(const std::string &regex) {
    NET4CXX_ASSERT_MSG(!reading(), "Already reading");
    _readRegex = boost::regex(regex);
    tryInlineRead();
}

void IOStream::readUntil(std::string delimiter) {
    NET4CXX_ASSERT_MSG(!reading(), "Already reading");
    _readDelimiter = std::move(delimiter);
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
    } else if (_readBytes) {
        if (_readBuffer.getActiveSize() >= _readBytes.get()) {
            size_t numBytes = _readBytes.get();
            _readBytes = boost::none;
            dataRead(_readBuffer.getReadPointer(), numBytes);
            _readBuffer.readCompleted(numBytes);
        }
    } else if (_readDelimiter) {
        const char *loc = StrNStr((const char *)_readBuffer.getReadPointer(), _readBuffer.getActiveSize(),
                                  _readDelimiter->c_str());
        if (loc) {
            size_t readBytes = loc - (const char *)_readBuffer.getReadPointer() + _readDelimiter->size();
            _readDelimiter = boost::none;
            dataRead(_readBuffer.getReadPointer(), readBytes);
            _readBuffer.readCompleted(readBytes);
        }
    } else if (_readRegex) {
        boost::cmatch m;
        if (boost::regex_search((const char *) _readBuffer.getReadPointer(),
                                (const char *) _readBuffer.getReadPointer() + _readBuffer.getActiveSize(), m,
                                _readRegex.get())) {
            auto readBytes = m.position((size_t) 0) + m.length();
            _readRegex = boost::none;
            dataRead(_readBuffer.getReadPointer(), (size_t)readBytes);
            _readBuffer.readCompleted((size_t)readBytes);
        }
    }
}


NS_END