//
// Created by yuwenyong on 17-9-14.
//

#ifndef NET4CXX_COMMON_SERIALIZATION_IARCHIVE_H
#define NET4CXX_COMMON_SERIALIZATION_IARCHIVE_H

#include "net4cxx/common/common.h"
#include <boost/endian/conversion.hpp>
#include "net4cxx/common/utilities/errors.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(ArchiveError, Exception);
NET4CXX_DECLARE_EXCEPTION(ArchivePositionError, ArchiveError);


class IArchive {
public:
    IArchive(const Byte *data, size_t size)
            : _storage(data)
            , _size(size) {

    }

    explicit IArchive(const ByteArray &data)
            : _storage(data.data())
            , _size(data.size()) {

    }

    bool empty() const {
        return _pos == _size;
    }

    size_t size() const {
        return _size;
    }

    size_t remainSize() const {
        return _size - _pos;
    }

    void ignore(size_t skip) {
        checkSkipOverflow(skip);
        _pos += skip;
    }

    IArchive& operator>>(bool &value) {
        value = read<char>() > 0;
        return *this;
    }

    IArchive& operator>>(uint8_t &value) {
        value = read<uint8_t>();
        return *this;
    }

    IArchive& operator>>(uint16_t &value) {
        value = read<uint16_t>();
        return *this;
    }

    IArchive& operator>>(uint32_t &value) {
        value = read<uint32_t>();
        return *this;
    }

    IArchive& operator>>(uint64_t &value) {
        value = read<uint64_t>();
        return *this;
    }

    IArchive& operator>>(int8_t &value) {
        value = read<int8_t>();
        return *this;
    }

    IArchive& operator>>(int16_t &value) {
        value = read<int16_t>();
        return *this;
    }

    IArchive& operator>>(int32_t &value) {
        value = read<int32_t>();
        return *this;
    }

    IArchive& operator>>(int64_t &value) {
        value = read<int64_t>();
        return *this;
    }

    IArchive& operator>>(float &value);

    IArchive& operator>>(double &value);

    IArchive& operator>>(std::string &value);

    IArchive& operator>>(ByteArray &value);

    template <size_t LEN>
    IArchive& operator>>(std::array<char, LEN> &value) {
        read((Byte *)value.data(), value.size());
        return *this;
    }

    template <size_t LEN>
    IArchive& operator>>(std::array<Byte, LEN> &value) {
        read(value.data(), value.size());
        return *this;
    }

    template <typename ElemT, size_t LEN>
    IArchive& operator>>(std::array<ElemT, LEN> &value) {
        for (auto &elem: value) {
            *this >> elem;
        }
        return *this;
    }

    template <typename ElemT, typename AllocT>
    IArchive& operator>>(std::vector<ElemT, AllocT> &value) {
        readSequence(value);
        return *this;
    }

    template <typename ElemT, typename AllocT>
    IArchive& operator>>(std::list<ElemT, AllocT> &value) {
        readSequence(value);
        return *this;
    }

    template <typename ElemT, typename AllocT>
    IArchive& operator>>(std::deque<ElemT, AllocT> &value) {
        readSequence(value);
        return *this;
    }

    template <typename KeyT, typename CompareT, typename AllocT>
    IArchive& operator>>(std::set<KeyT, CompareT, AllocT> &value) {
        readSet(value);
        return *this;
    };

    template <typename KeyT, typename CompareT, typename AllocT>
    IArchive& operator>>(std::multiset<KeyT, CompareT, AllocT> &value) {
        readSet(value);
        return *this;
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT>
    IArchive& operator>>(std::unordered_set<KeyT, HashT, PredT, AllocT> &value) {
        readSet(value);
        return *this;
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT>
    IArchive& operator>>(std::unordered_multiset<KeyT, HashT, PredT, AllocT> &value) {
        readSet(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT>
    IArchive& operator>>(std::map<KeyT, ValueT, CompareT, AllocT> &value) {
        readMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT>
    IArchive& operator>>(std::multimap<KeyT, ValueT, CompareT, AllocT> &value) {
        readMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT>
    IArchive& operator>>(std::unordered_map<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        readMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT>
    IArchive& operator>>(std::unordered_multimap<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        readMapping(value);
        return *this;
    };

    template <typename ValueT>
    IArchive& operator>>(ValueT &value) {
        value.serialize(*this);
        return *this;
    }

    template <typename ValueT>
    IArchive& operator&(ValueT &value) {
        return *this >> value;
    }
protected:
    template <typename ElemT, typename AllocT, template <typename, typename> class ContainerT>
    void readSequence(ContainerT<ElemT, AllocT> &value) {
        size_t len = read<uint8_t>();
        if (len == 0xFF) {
            len = read<uint32_t>();
        }
        ContainerT<ElemT, AllocT> result;
        ElemT elem;
        for (size_t i = 0; i != len; ++i) {
            *this >> elem;
            result.emplace_back(std::move(elem));
        }
        result.swap(value);
    }

    template <typename ElemT, typename CompareT, typename AllocT,
            template <typename, typename, typename > class ContainerT>
    void readSet(ContainerT<ElemT, CompareT, AllocT> &value) {
        size_t len = read<uint8_t>();
        if (len == 0xFF) {
            len = read<uint32_t>();
        }
        ContainerT<ElemT, CompareT, AllocT> result;
        for (size_t i = 0; i != len; ++i) {
            ElemT elem;
            *this >> elem;
            result.emplace(std::move(elem));
        }
        result.swap(value);
    }

    template <typename ElemT, typename HashT, typename PredT, typename AllocT,
            template <typename, typename, typename, typename> class ContainerT>
    void readSet(ContainerT<ElemT, HashT, PredT, AllocT> &value) {
        size_t len = read<uint8_t>();
        if (len == 0xFF) {
            len = read<uint32_t>();
        }
        ContainerT<ElemT, HashT, PredT, AllocT> result;
        for (size_t i = 0; i != len; ++i) {
            ElemT elem;
            *this >> elem;
            result.emplace(std::move(elem));
        }
        result.swap(value);
    }

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT,
            template <typename, typename, typename, typename> class ContainerT>
    void readMapping(ContainerT<KeyT, ValueT, CompareT, AllocT> &value) {
        size_t len = read<uint8_t>();
        if (len == 0xFF) {
            len = read<uint32_t>();
        }
        ContainerT<KeyT, ValueT, CompareT, AllocT> result;
        for (size_t i = 0; i != len; ++i) {
            KeyT key;
            ValueT val;
            *this >> key >> val;
            result.emplace(std::move(key), std::move(val));
        }
        result.swap(value);
    }

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT,
            template <typename, typename, typename, typename, typename> class ContainerT>
    void readMapping(ContainerT<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        size_t len = read<uint8_t>();
        if (len == 0xFF) {
            len = read<uint32_t>();
        }
        ContainerT<KeyT, ValueT, HashT, PredT, AllocT> result;
        for (size_t i = 0; i != len; ++i) {
            KeyT key;
            ValueT val;
            *this >> key >> val;
            result.emplace(std::move(key), std::move(val));
        }
        result.swap(value);
    }

    template <typename ValueT>
    ValueT read() {
        checkReadOverflow(sizeof(ValueT));
        ValueT val;
        memcpy(&val, _storage + _pos, sizeof(ValueT));
        val = boost::endian::little_to_native(val);
        _pos += sizeof(ValueT);
        return val;
    }

    void read(Byte *dest, size_t len) {
        checkReadOverflow(len);
        std::memcpy(dest, _storage + _pos, len);
        _pos += len;
    }

    void checkReadOverflow(size_t len) const;

    void checkSkipOverflow(size_t len) const;

    const Byte *_storage;
    size_t _pos{0};
    size_t _size;
};

NS_END

#endif //NET4CXX_COMMON_SERIALIZATION_IARCHIVE_H
