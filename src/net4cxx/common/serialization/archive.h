//
// Created by yuwenyong.vincent on 2018/7/8.
//

#ifndef NET4CXX_COMMON_SERIALIZATION_ARCHIVE_H
#define NET4CXX_COMMON_SERIALIZATION_ARCHIVE_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/utilities/errors.h"

NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(ArchiveError, Exception);
NET4CXX_DECLARE_EXCEPTION(ArchivePositionError, ArchiveError);


enum class ArchiveMode {
    Write,
    Read
};


template<typename ArchiveT>
class ArchiveModeGuard {
public:
    ArchiveModeGuard(ArchiveT *archive, ArchiveMode mode)
            : _archive(archive)
            , _mode(archive->getMode()) {
        _archive->setMode(mode);
    }

    ~ArchiveModeGuard() {
        _archive->setMode(_mode);
    }
protected:
    ArchiveT *_archive;
    ArchiveMode _mode;
};


class NET4CXX_COMMON_API Archive {
public:
    Archive() = default;

    explicit Archive(size_t reserved) {
        _storage.reserve(reserved);
    }

    Archive(const Byte *data, size_t size)
            : _mode{ArchiveMode::Read}
            , _storage(data, data + size) {

    }

    explicit Archive(ByteArray data)
            : _mode{ArchiveMode::Read}
            , _storage(std::move(data)){

    }

    Archive(const Archive &rhs) = default;

    Archive(Archive &&rhs) noexcept
            : _mode{rhs._mode}
            , _pos{rhs._pos}
            , _storage{std::move(rhs._storage)} {
        rhs._pos = 0;
    }

    Archive& operator=(const Archive &rhs) = default;

    Archive& operator=(Archive &&rhs) noexcept {
        if (this != &rhs) {
            _mode = rhs._mode;
            _pos = rhs._pos;
            _storage = std::move(rhs._storage);
            rhs._pos = 0;
        }
        return *this;
    }

    bool empty() const {
        return _pos == _storage.size();
    }

    size_t size() const {
        return _storage.size() - _pos;
    }

    Byte* contents() {
        if (empty()) {
            NET4CXX_THROW_EXCEPTION(ArchiveError, "Empty buffer");
        }
        return _storage.data() + _pos;
    }

    const Byte* contents() const {
        if (empty()) {
            NET4CXX_THROW_EXCEPTION(ArchiveError, "Empty buffer");
        }
        return _storage.data() + _pos;
    }

    bool storageEmtpy() const {
        return _storage.empty();
    }

    size_t storageSize() const {
        return _storage.size();
    }

    Byte* storageContents() {
        if (storageEmtpy()) {
            NET4CXX_THROW_EXCEPTION(ArchiveError, "Empty buffer");
        }
        return _storage.data();
    }

    const Byte* storageContents() const {
        if (storageEmtpy()) {
            NET4CXX_THROW_EXCEPTION(ArchiveError, "Empty buffer");
        }
        return _storage.data();
    }

    ArchiveMode getMode() const {
        return _mode;
    }

    void setMode(ArchiveMode mode) {
        _mode = mode;
    }

    void setWriteMode() {
        _mode = ArchiveMode::Write;
    }

    void setReadMode() {
        _mode = ArchiveMode::Read;
    }

    void ignore(size_t skip) {
        checkSkipOverflow(skip);
        _pos += skip;
    }

    Archive& operator<<(uint8_t value) {
        append<uint8_t>(value);
        return *this;
    }

    Archive& operator<<(uint16_t value) {
        append<uint16_t>(value);
        return *this;
    }

    Archive& operator<<(uint32_t value) {
        append<uint32_t>(value);
        return *this;
    }

    Archive& operator<<(uint64_t value) {
        append<uint64_t>(value);
        return *this;
    }

    Archive& operator<<(int8_t value) {
        append<int8_t>(value);
        return *this;
    }

    Archive& operator<<(int16_t value) {
        append<int16_t>(value);
        return *this;
    }

    Archive& operator<<(int32_t value) {
        append<int32_t>(value);
        return *this;
    }

    Archive& operator<<(int64_t value) {
        append<int64_t>(value);
        return *this;
    }

    Archive& operator<<(float value) {
        append<float>(value);
        return *this;
    }

    Archive& operator<<(double value) {
        append<double>(value);
        return *this;
    }

    Archive& operator<<(const char *value);

    Archive& operator<<(std::string &value);

    Archive& operator<<(const ByteArray &value);

    template <size_t LEN>
    Archive& operator<<(const std::array<char, LEN> &value) {
        append((const Byte *)value.data(), value.size());
        return *this;
    }

    template <size_t LEN>
    Archive& operator<<(const std::array<Byte, LEN> &value) {
        append(value.data(), value.size());
        return *this;
    }

    template <typename ElemT, size_t LEN>
    Archive& operator<<(std::array<ElemT, LEN> &value) {
        for (auto &elem: value) {
            *this << elem;
        }
        return *this;
    }

    template <typename ElemT, typename AllocT>
    Archive& operator<<(std::vector<ElemT, AllocT> &value) {
        appendSequence(value);
        return *this;
    }

    template <typename ElemT, typename AllocT>
    Archive& operator<<(std::list<ElemT, AllocT> &value) {
        appendSequence(value);
        return *this;
    }

    template <typename ElemT, typename AllocT>
    Archive& operator<<(std::deque<ElemT, AllocT> &value) {
        appendSequence(value);
        return *this;
    }

    template <typename KeyT, typename CompareT, typename AllocT>
    Archive& operator<<(std::set<KeyT, CompareT, AllocT> &value) {
        appendSet(value);
        return *this;
    };

    template <typename KeyT, typename CompareT, typename AllocT>
    Archive& operator<<(std::multiset<KeyT, CompareT, AllocT> &value) {
        appendSet(value);
        return *this;
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT>
    Archive& operator<<(std::unordered_set<KeyT, HashT, PredT, AllocT> &value) {
        appendSet(value);
        return *this;
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT>
    Archive& operator<<(std::unordered_multiset<KeyT, HashT, PredT, AllocT> &value) {
        appendSet(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT>
    Archive& operator<<(std::map<KeyT, ValueT, CompareT, AllocT> &value) {
        appendMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT>
    Archive& operator<<(std::multimap<KeyT, ValueT, CompareT, AllocT> &value) {
        appendMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT>
    Archive& operator<<(std::unordered_map<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        appendMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT>
    Archive& operator<<(std::unordered_multimap<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        appendMapping(value);
        return *this;
    };

    template <typename ValueT>
    Archive& operator<<(boost::optional<ValueT> &value) {
        if (value) {
            append<uint8>(0x01);
            *this << *value;
        } else {
            append<uint8>(0x00);
        }
        return *this;
    }

    template <typename ValueT>
    Archive& operator<<(ValueT &value) {
        ArchiveModeGuard<Archive> guard(this, ArchiveMode::Write);
        value.serialize(*this);
        return *this;
    }

    Archive& operator>>(bool &value) {
        value = read<char>() > 0;
        return *this;
    }

    Archive& operator>>(uint8_t &value) {
        value = read<uint8_t>();
        return *this;
    }

    Archive& operator>>(uint16_t &value) {
        value = read<uint16_t>();
        return *this;
    }

    Archive& operator>>(uint32_t &value) {
        value = read<uint32_t>();
        return *this;
    }

    Archive& operator>>(uint64_t &value) {
        value = read<uint64_t>();
        return *this;
    }

    Archive& operator>>(int8_t &value) {
        value = read<int8_t>();
        return *this;
    }

    Archive& operator>>(int16_t &value) {
        value = read<int16_t>();
        return *this;
    }

    Archive& operator>>(int32_t &value) {
        value = read<int32_t>();
        return *this;
    }

    Archive& operator>>(int64_t &value) {
        value = read<int64_t>();
        return *this;
    }

    Archive& operator>>(float &value);

    Archive& operator>>(double &value);

    Archive& operator>>(std::string &value);

    Archive& operator>>(ByteArray &value);

    template <size_t LEN>
    Archive& operator>>(std::array<char, LEN> &value) {
        read((Byte *)value.data(), value.size());
        return *this;
    }

    template <size_t LEN>
    Archive& operator>>(std::array<Byte, LEN> &value) {
        read(value.data(), value.size());
        return *this;
    }

    template <typename ElemT, size_t LEN>
    Archive& operator>>(std::array<ElemT, LEN> &value) {
        for (auto &elem: value) {
            *this >> elem;
        }
        return *this;
    }

    template <typename ElemT, typename AllocT>
    Archive& operator>>(std::vector<ElemT, AllocT> &value) {
        readSequence(value);
        return *this;
    }

    template <typename ElemT, typename AllocT>
    Archive& operator>>(std::list<ElemT, AllocT> &value) {
        readSequence(value);
        return *this;
    }

    template <typename ElemT, typename AllocT>
    Archive& operator>>(std::deque<ElemT, AllocT> &value) {
        readSequence(value);
        return *this;
    }

    template <typename KeyT, typename CompareT, typename AllocT>
    Archive& operator>>(std::set<KeyT, CompareT, AllocT> &value) {
        readSet(value);
        return *this;
    };

    template <typename KeyT, typename CompareT, typename AllocT>
    Archive& operator>>(std::multiset<KeyT, CompareT, AllocT> &value) {
        readSet(value);
        return *this;
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT>
    Archive& operator>>(std::unordered_set<KeyT, HashT, PredT, AllocT> &value) {
        readSet(value);
        return *this;
    };

    template <typename KeyT, typename HashT, typename PredT, typename AllocT>
    Archive& operator>>(std::unordered_multiset<KeyT, HashT, PredT, AllocT> &value) {
        readSet(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT>
    Archive& operator>>(std::map<KeyT, ValueT, CompareT, AllocT> &value) {
        readMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename CompareT, typename AllocT>
    Archive& operator>>(std::multimap<KeyT, ValueT, CompareT, AllocT> &value) {
        readMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT>
    Archive& operator>>(std::unordered_map<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        readMapping(value);
        return *this;
    };

    template <typename KeyT, typename ValueT, typename HashT, typename PredT, typename AllocT>
    Archive& operator>>(std::unordered_multimap<KeyT, ValueT, HashT, PredT, AllocT> &value) {
        readMapping(value);
        return *this;
    };

    template <typename ValueT>
    Archive& operator>>(boost::optional<ValueT> &value) {
        auto hasValue = read<uint8_t>();
        if (hasValue == 0) {
            value = boost::none;
        } else {
            ValueT val;
            *this >> val;
            value = std::move(val);
        }
        return *this;
    }

    template <typename ValueT>
    Archive& operator>>(ValueT &value) {
        ArchiveModeGuard<Archive> guard(this, ArchiveMode::Read);
        value.serialize(*this);
        return *this;
    }

    template <typename ValueT>
    Archive& operator&(ValueT &value) {
        if (_mode == ArchiveMode::Write) {
            return *this << value;
        } else {
            return *this >> value;
        }
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
        append((uint8 *)&value, sizeof(value));
    }

    void append(const Byte *src, size_t len) {
        _storage.insert(_storage.end(), src, src + len);
    }

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
        memcpy(&val, _storage.data() + _pos, sizeof(ValueT));
        _pos += sizeof(ValueT);
        return val;
    }

    void read(Byte *dest, size_t len) {
        checkReadOverflow(len);
        std::memcpy(dest, _storage.data() + _pos, len);
        _pos += len;
    }

    void checkReadOverflow(size_t len) const;

    void checkSkipOverflow(size_t len) const;

    ArchiveMode _mode{ArchiveMode::Write};
    size_t _pos{0};
    ByteArray _storage;
};

NS_END

#endif //NET4CXX_COMMON_SERIALIZATION_ARCHIVE_H
