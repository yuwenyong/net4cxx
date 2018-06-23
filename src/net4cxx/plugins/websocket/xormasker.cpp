//
// Created by yuwenyong.vincent on 2018/5/19.
//

#include "net4cxx/plugins/websocket/xormasker.h"

NS_BEGIN

uint64_t XorMaskerNull::pointer() const {
    return _ptr;
}

void XorMaskerNull::reset() {
    _ptr = 0;
}

void XorMaskerNull::process(Byte *data, uint64_t len) {
    _ptr += len;
}


uint64_t XorMaskerSimple::pointer() const {
    return _ptr;
}

void XorMaskerSimple::reset() {
    _ptr = 0;
}

void XorMaskerSimple::process(Byte *data, uint64_t len) {
    for (size_t k = 0; k < len; ++k) {
        data[k] ^= _msk[_ptr & 3u];
        ++_ptr;
    }
}


uint64_t XorMaskerShifted1::pointer() const {
    return _ptr;
}

void XorMaskerShifted1::reset() {
    _ptr = 0;
}

void XorMaskerShifted1::process(Byte *data, uint64_t len) {
    auto &msk = _mskarray[_ptr & 3u];
    for (size_t k = 0; k < len; ++k) {
        data[k] ^= msk[k & 3u];
    }
    _ptr += len;
}



NS_END