//
// Created by yuwenyong on 17-9-13.
//

#ifndef NET4CXX_COMMON_UTILITIES_MESSAGEBUFFER_H
#define NET4CXX_COMMON_UTILITIES_MESSAGEBUFFER_H

#include "net4cxx/common/common.h"

NS_BEGIN

class NET4CXX_COMMON_API MessageBuffer {
public:
    MessageBuffer()
            : _wpos(0)
            , _rpos(0)
            , _storage() {
        _storage.resize(4096);
    }

    explicit MessageBuffer(size_t initialSize)
            : _wpos(0)
            , _rpos(0)
            , _storage() {
        _storage.resize(initialSize);
    }

    void reset() {
        _wpos = 0;
        _rpos = 0;
    }

    void resize(size_t bytes) {
        _storage.resize(bytes);
    }

    Byte* getBasePointer() {
        return _storage.data();
    }

    const Byte* getBasePointer() const {
        return _storage.data();
    }

    Byte* getReadPointer() {
        return getBasePointer() + _rpos;
    }

    const Byte* getReadPointer() const {
        return getBasePointer() + _rpos;
    }

    Byte* getWritePointer() {
        return getBasePointer() + _wpos;
    }

    const Byte* getWritePointer() const {
        return getBasePointer() + _wpos;
    }

    void readCompleted(size_t bytes) {
        _rpos += bytes;
    }

    void writeCompleted(size_t bytes) {
        _wpos += bytes;
    }

    size_t getActiveSize() const {
        return _wpos - _rpos;
    }

    size_t getRemainingSpace() const {
        return _storage.size() - _wpos;
    }

    size_t getBufferSize() const {
        return _storage.size();
    }

    void normalize() {
        if (_rpos) {
            if (_rpos != _wpos) {
                memmove(getBasePointer(), getReadPointer(), getActiveSize());
            }
            _wpos -= _rpos;
            _rpos = 0;
        }
    }

    void ensureFreeSpace() {
        if (getRemainingSpace() == 0) {
            _storage.resize(_storage.size() * 3 / 2);
        }
    }

    void ensureFreeSpace(size_t space) {
        if (getRemainingSpace() < space) {
            _storage.resize(getBufferSize() + space - getRemainingSpace());
        }
    }

    void write(const Byte* data, size_t size) {
        if (size) {
            memcpy(getWritePointer(), data, size);
            writeCompleted(size);
        }
    }
protected:
    size_t _wpos;
    size_t _rpos;
    ByteArray _storage;
};

NS_END


#endif //NET4CXX_COMMON_UTILITIES_MESSAGEBUFFER_H
