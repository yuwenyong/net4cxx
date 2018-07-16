//
// Created by yuwenyong.vincent on 2018/7/8.
//

#include "net4cxx/common/serialization/bearchive.h"


NS_BEGIN

BEArchive& BEArchive::operator<<(const char *value) {
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

BEArchive& BEArchive::operator<<(std::string &value) {
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

BEArchive& BEArchive::operator<<(const ByteArray &value) {
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

BEArchive& BEArchive::operator>>(float &value) {
    value = readReal<float>();
    if (!std::isfinite(value)) {
        NET4CXX_THROW_EXCEPTION(ArchiveError, "Infinite float value");
    }
    return *this;
}

BEArchive& BEArchive::operator>>(double &value) {
    value = readReal<double>();
    if (!std::isfinite(value)) {
        NET4CXX_THROW_EXCEPTION(ArchiveError, "Infinite double value");
    }
    return *this;
}

BEArchive& BEArchive::operator>>(std::string &value) {
    size_t len = read<uint8_t>();
    if (len == 0xFF) {
        len = read<uint32_t>();
    }
    std::string result;
    if (len > 0) {
        result.resize(len);
        read((Byte *)result.data(), len);
    }
    value.swap(result);
    return *this;
}

BEArchive& BEArchive::operator>>(ByteArray &value) {
    size_t len = read<uint8_t>();
    if (len == 0xFF) {
        len = read<uint32_t>();
    }
    ByteArray result;
    if (len > 0) {
        result.resize(len);
        read(result.data(), len);
    }
    value.swap(result);
    return *this;
}

void BEArchive::checkSkipOverflow(size_t len) const {
    if (_pos + len > _storage.size()) {
        NET4CXX_THROW_EXCEPTION(ArchivePositionError,
                                "Attempted to skip value with size: %lu in BEArchive (pos: %lu size: %lu)",
                                len, _pos, _storage.size());
    }
}

void BEArchive::checkReadOverflow(size_t len) const {
    if (_pos + len > _storage.size()) {
        NET4CXX_THROW_EXCEPTION(ArchivePositionError,
                                "Attempted to skip value with size: %lu in BEArchive (pos: %lu size: %lu)",
                                len, _pos, _storage.size());
    }
}

NS_END
