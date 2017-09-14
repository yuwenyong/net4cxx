//
// Created by yuwenyong on 17-9-14.
//

#include "net4cxx/common/serialization/iarchive.h"
#include "net4cxx/common/utilities/strutil.h"

NS_BEGIN


IArchive& IArchive::operator>>(float &value) {
    value = read<float>();
    if (!std::isfinite(value)) {
        NET4CXX_THROW_EXCEPTION(ArchiveError, "Infinite float value");
    }
    return *this;
}

IArchive& IArchive::operator>>(double &value) {
    value = read<double>();
    if (!std::isfinite(value)) {
        NET4CXX_THROW_EXCEPTION(ArchiveError, "Infinite double value");
    }
    return *this;
}

IArchive& IArchive::operator>>(std::string &value) {
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

IArchive& IArchive::operator>>(ByteArray &value) {
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

void IArchive::checkSkipOverflow(size_t len) const {
    if (_pos + len > _size) {
        std::string error = StrUtil::format("Attempted to skip value with size: %lu in IArchive (pos: %lu size: %lu)",
                                            len, _pos, _size);
        NET4CXX_THROW_EXCEPTION(ArchivePositionError, error);
    }
}

void IArchive::checkReadOverflow(size_t len) const {
    if (_pos + len > _size) {
        std::string error = StrUtil::format("Attempted to skip value with size: %lu in IArchive (pos: %lu size: %lu)",
                                            len, _pos, _size);
        NET4CXX_THROW_EXCEPTION(ArchivePositionError, error);
    }
}

NS_END