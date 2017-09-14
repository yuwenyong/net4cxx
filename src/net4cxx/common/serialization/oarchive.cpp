//
// Created by yuwenyong on 17-9-14.
//

#include "net4cxx/common/serialization/oarchive.h"

NS_BEGIN

OArchive& OArchive::operator<<(const char *value) {
    size_t len = strlen(value);
    if (len >= 0xff) {
        append<uint8_t>(0xff);
        append<uint32_t>(static_cast<uint32_t>(len));
    } else {
        append<uint8_t>(static_cast<uint8_t>(len));
    }
    append((const Byte *)value, len);
    return *this;
}

OArchive& OArchive::operator<<(std::string &value) {
    size_t len = value.size();
    if (len >= 0xff) {
        append<uint8_t>(0xff);
        append<uint32_t>(static_cast<uint32_t>(len));
    } else {
        append<uint8_t>(static_cast<uint8_t>(len));
    }
    append((const Byte *)value.data(), len);
    return *this;
}

OArchive& OArchive::operator<<(const ByteArray &value) {
    size_t len = value.size();
    if (len >= 0xff) {
        append<uint8_t>(0xff);
        append<uint32_t>(static_cast<uint32_t>(len));
    } else {
        append<uint8_t>(static_cast<uint8_t>(len));
    }
    append(value.data(), len);
    return *this;
}

NS_END