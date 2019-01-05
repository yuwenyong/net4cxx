//
// Created by yuwenyong.vincent on 2019-01-05.
//

#ifndef NET4CXX_COMMON_SERIALIZATION_BYTECONVERT_H
#define NET4CXX_COMMON_SERIALIZATION_BYTECONVERT_H

#include "net4cxx/common/common.h"
#include <boost/endian/conversion.hpp>


NS_BEGIN


struct ByteOrderNative {

};


struct ByteOrderNetwork {

};


struct ByteOrderLittleEndian {

};


struct ByteOrderBigEndian {

};



class NET4CXX_COMMON_API ByteConvert {
public:
    static int8_t convertTo(int8_t val, ByteOrderNative) {
        return val;
    }
    
    static int8_t convertTo(int8_t val, ByteOrderNetwork) {
        return val;
    }
    
    static int8_t convertTo(int8_t val, ByteOrderLittleEndian) {
        return val;
    }
    
    static int8_t convertTo(int8_t val, ByteOrderBigEndian) {
        return val;
    }

    static int8_t convertFrom(int8_t val, ByteOrderNative) {
        return val;
    }

    static int8_t convertFrom(int8_t val, ByteOrderNetwork) {
        return val;
    }

    static int8_t convertFrom(int8_t val, ByteOrderLittleEndian) {
        return val;
    }

    static int8_t convertFrom(int8_t val, ByteOrderBigEndian) {
        return val;
    }

    static uint8_t convertTo(uint8_t val, ByteOrderNative) {
        return val;
    }

    static uint8_t convertTo(uint8_t val, ByteOrderNetwork) {
        return val;
    }

    static uint8_t convertTo(uint8_t val, ByteOrderLittleEndian) {
        return val;
    }

    static uint8_t convertTo(uint8_t val, ByteOrderBigEndian) {
        return val;
    }

    static uint8_t convertFrom(uint8_t val, ByteOrderNative) {
        return val;
    }

    static uint8_t convertFrom(uint8_t val, ByteOrderNetwork) {
        return val;
    }

    static uint8_t convertFrom(uint8_t val, ByteOrderLittleEndian) {
        return val;
    }

    static uint8_t convertFrom(uint8_t val, ByteOrderBigEndian) {
        return val;
    }
    
    
    template <typename ValueT>
    static ValueT convertTo(ValueT val, ByteOrderNative) {
        return val;
    }

    template <typename ValueT>
    static ValueT convertTo(ValueT val, ByteOrderNetwork) {
        return boost::endian::native_to_big(val);
    }

    template <typename ValueT>
    static ValueT convertTo(ValueT val, ByteOrderLittleEndian) {
        return boost::endian::native_to_little(val);
    }

    template <typename ValueT>
    static ValueT convertTo(ValueT val, ByteOrderBigEndian) {
        return boost::endian::native_to_big(val);
    }

    template <typename ValueT>
    static ValueT convertFrom(ValueT val, ByteOrderNative) {
        return val;
    }

    template <typename ValueT>
    static ValueT convertFrom(ValueT val, ByteOrderNetwork) {
        return boost::endian::big_to_native(val);
    }

    template <typename ValueT>
    static ValueT convertFrom(ValueT val, ByteOrderLittleEndian) {
        return boost::endian::little_to_native(val);
    }

    template <typename ValueT>
    static ValueT convertFrom(ValueT val, ByteOrderBigEndian) {
        return boost::endian::big_to_native(val);
    }
};


NS_END

#endif //NET4CXX_COMMON_SERIALIZATION_BYTECONVERT_H
