//
// Created by yuwenyong on 17-9-14.
//

#ifndef NET4CXX_COMMON_SERIALIZATION_OARCHIVE_H
#define NET4CXX_COMMON_SERIALIZATION_OARCHIVE_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/serialization/iarchive.h"


NS_BEGIN


class OArchive {
public:
    OArchive() = default;

    explicit OArchive(size_t reserve) {
        _storage.reserve(reserve);
    }

    ByteArray&& move() noexcept {
        return std::move(_storage);
    }

    bool empty() const {
        return _storage.empty();
    }

    size_t size() const {
        return _storage.size();
    }

    Byte* contents() {
        if (_storage.empty()) {
            NET4CXX_THROW_EXCEPTION(ArchiveError, "Empty buffer");
        }
        return _storage.data();
    }

    const Byte* contents() const {
        if (_storage.empty()) {
            NET4CXX_THROW_EXCEPTION(ArchiveError, "Empty buffer");
        }
        return _storage.data();
    }

    OArchive& operator<<(uint8_t value) {
        append<uint8_t>(value);
        return *this;
    }

    OArchive& operator<<(uint16_t value) {
        append<uint16_t>(value);
        return *this;
    }

    OArchive& operator<<(uint32_t value) {
        append<uint32_t>(value);
        return *this;
    }

    OArchive& operator<<(uint64_t value) {
        append<uint64_t>(value);
        return *this;
    }

    OArchive& operator<<(int8_t value) {
        append<int8_t>(value);
        return *this;
    }

    OArchive& operator<<(int16_t value) {
        append<int16_t>(value);
        return *this;
    }

    OArchive& operator<<(int32_t value) {
        append<int32_t>(value);
        return *this;
    }

    OArchive& operator<<(int64_t value) {
        append<int64_t>(value);
        return *this;
    }

    OArchive& operator<<(float value) {
        append<float>(value);
        return *this;
    }

    OArchive& operator<<(double value) {
        append<double>(value);
        return *this;
    }

    OArchive& operator<<(const char *value);

    OArchive& operator<<(std::string &value);

    OArchive& operator<<(const ByteArray &value);

    template <size_t LEN>
    OArchive& operator<<(const std::array<char, LEN> &value) {
        append((const Byte *)value.data(), value.size());
        return *this;
    }

    template <size_t LEN>
    OArchive& operator<<(const std::array<Byte, LEN> &value) {
        append(value.data(), value.size());
        return *this;
    }

    template <typename ElemT, size_t LEN>
    OArchive& operator<<(std::array<ElemT, LEN> &value) {
        for (auto &elem: value) {
            *this << elem;
        }
        return *this;
    }

    template <typename ElemT, typename AllocT>
    OArchive& operator<<(std::vector<ElemT, AllocT> &value) {
        appendSequence(value);
        return *this;
    }

    template <typename ElemT, typename AllocT>
    OArchive& operator<<(std::list<ElemT, AllocT> &value) {
        appendSequence(value);
        return *this;
    }

    template <typename ElemT, typename AllocT>
    OArchive& operator<<(std::deque<ElemT, AllocT> &value) {
        appendSequence(value);
        return *this;
    }

    template <typename KeyT, typename CompareT, typename AllocT>
    OArchive& operator<<(std::set<KeyT, CompareT, AllocT> &value) {
        appendSet(value);
        return *this;
    };

    template <typename KeyT, typename CompareT, typename AllocT>
    OArchive& operator<<(std::multiset<KeyT, CompareT, AllocT> &value) {
        appendSet(value);
        return *this;
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT>
    OArchive& operator<<(std::unordered_set<KeyT, HashT, PredT, AllocT> &value) {
        appendSet(value);
        return *this;
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT>
    OArchive& operator<<(std::unordered_multiset<KeyT, HashT, PredT, AllocT> &value) {
        appendSet(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT>
    OArchive& operator<<(std::map<KeyT, ValueT, CompareT, AllocT> &value) {
        appendMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT>
    OArchive& operator<<(std::multimap<KeyT, ValueT, CompareT, AllocT> &value) {
        appendMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT>
    OArchive& operator<<(std::unordered_map<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        appendMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT>
    OArchive& operator<<(std::unordered_multimap<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        appendMapping(value);
        return *this;
    };

    template <typename ValueT>
    OArchive& operator<<(ValueT &value) {
        value.serialize(*this);
        return *this;
    }

    template <typename ValueT>
    OArchive& operator&(ValueT &value) {
        return *this << value;
    }
protected:
    template <typename ElemT, typename AllocT, template <typename, typename > class ContainerT>
    void appendSequence(ContainerT<ElemT, AllocT> &value) {
        size_t len = value.size();
        if (len >= 0xFF) {
            append<uint8>(0xff);
            append<uint32>(static_cast<uint32>(len));
        } else {
            append<uint8>(static_cast<uint8>(len));
        }
        for (ElemT &elem: value) {
            *this << elem;
        }
    }

    template <typename KeyT, typename CompareT, typename AllocT,
            template <typename, typename, typename > class ContainerT>
    void appendSet(ContainerT<KeyT, CompareT, AllocT> &value) {
        size_t len = value.size();
        if (len >= 0xFF) {
            append<uint8>(0xff);
            append<uint32>(static_cast<uint32>(len));
        } else {
            append<uint8>(static_cast<uint8>(len));
        }
        for (const KeyT &key: value) {
            *this << key;
        }
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT,
            template <typename, typename, typename, typename > class ContainerT>
    void appendSet(ContainerT<KeyT, HashT, PredT, AllocT> &value) {
        size_t len = value.size();
        if (len >= 0xFF) {
            append<uint8>(0xff);
            append<uint32>(static_cast<uint32>(len));
        } else {
            append<uint8>(static_cast<uint8>(len));
        }
        for (const KeyT &key: value) {
            *this << key;
        }
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT,
            template <typename, typename, typename, typename> class ContainerT>
    void appendMapping(ContainerT<KeyT, ValueT, CompareT, AllocT> &value) {
        size_t len = value.size();
        if (len >= 0xFF) {
            append<uint8>(0xff);
            append<uint32>(static_cast<uint32>(len));
        } else {
            append<uint8>(static_cast<uint8>(len));
        }
        for (auto &kv: value) {
            *this << kv.first << kv.second;
        }
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT,
            template <typename, typename, typename, typename, typename > class ContainerT>
    void appendMapping(ContainerT<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        size_t len = value.size();
        if (len >= 0xFF) {
            append<uint8>(0xff);
            append<uint32>(static_cast<uint32>(len));
        } else {
            append<uint8>(static_cast<uint8>(len));
        }
        for (auto &kv: value) {
            *this << kv.first << kv.second;
        }
    };

    template <typename ValueT>
    void append(ValueT value) {
        static_assert(std::is_fundamental<ValueT>::value, "append(compound)");
        value = boost::endian::native_to_little(value);
        append((uint8 *)&value, sizeof(value));
    }

    void append(const Byte *src, size_t len) {
        _storage.insert(_storage.end(), src, src + len);
    }

    ByteArray _storage;
};


NS_END

#endif //NET4CXX_COMMON_SERIALIZATION_OARCHIVE_H
