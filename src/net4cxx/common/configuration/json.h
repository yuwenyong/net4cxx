//
// Created by yuwenyong on 17-12-4.
//

#ifndef NET4CXX_COMMON_CONFIGURATION_JSON_H
#define NET4CXX_COMMON_CONFIGURATION_JSON_H

#include "net4cxx/common/common.h"
#include <boost/variant.hpp>
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/utilities/util.h"

NS_BEGIN


enum class JSONType {
    nullValue = 0,
    intValue,
    uintValue,
    realValue,
    stringValue,
    boolValue,
    arrayValue,
    objectValue,
};


enum CommentPlacement {
    COMMENT_BEFORE = 0,
    COMMENT_ON_SAME_LINE,
    COMMENT_AFTER,
    COMMENT_COUNT,
};


class NET4CXX_COMMON_API JSONValue {
public:
    struct NullValue {

    };

    using ObjectType = std::map<std::string, JSONValue>;
    using ArrayType = std::vector<JSONValue>;
    using ValueType = boost::variant<NullValue, int64_t, uint64_t, double, std::string, bool, ArrayType, ObjectType>;
    using ArrayIterator = ArrayType::iterator;
    using ConstArrayIterator = ArrayType::const_iterator;
    using ObjectIterator = ObjectType::iterator;
    using ConstObjectIterator = ObjectType::const_iterator;

    friend bool operator<(const JSONValue &lhs, const JSONValue &rhs);
    friend bool operator==(const JSONValue &lhs, const JSONValue &rhs);

    JSONValue() = default;

    JSONValue(JSONType type);

    JSONValue(nullptr_t) {

    }

    JSONValue(int value): _value(int64_t{value}) {

    }

    JSONValue(unsigned int value): _value(uint64_t{value}) {

    }

    JSONValue(int64_t value): _value(value) {

    }

    JSONValue(uint64_t value): _value(value) {

    }

    JSONValue(float value): _value(double{value}) {

    }

    JSONValue(double value): _value(value) {

    }

    JSONValue(const char *value): _value(std::string{value}) {

    }

    JSONValue(const char *value, size_t length): _value(std::string{value, value + length}) {

    }

    JSONValue(const std::string &value): _value(value) {

    }

    JSONValue(bool value): _value(value) {

    }

    void swap(JSONValue &other) {
        std::swap(*this, other);
    }

    void swapPayload(JSONValue &other) {
        std::swap(_value, other._value);
    }

    void copy(const JSONValue &other) {
        *this = other;
    }

    void copyPayload(const JSONValue &other) {
        _value = other._value;
    }

    JSONType type() const;

    int compare(const JSONValue &other) const;

    std::string asString() const;

    int asInt() const;

    unsigned int asUInt() const;

    int64_t asInt64() const;


    uint64_t asUInt64() const;

    float asFloat() const;

    double asDouble() const;

    bool asBool() const;

    bool isNull() const {
        return _value.type() == typeid(NullValue);
    }

    bool isBool() const {
        return _value.type() == typeid(bool);
    }

    bool isInt() const;

    bool isInt64() const;

    bool isUInt() const;

    bool isUInt64() const;

    bool isIntegral() const;

    bool isDouble() const {
        return _value.type() == typeid(int64_t) || _value.type() == typeid(uint64_t) || _value.type() == typeid(double);
    }

    bool isNumeric() const {
        return isDouble();
    }

    bool isString() const {
        return _value.type() == typeid(std::string);
    }

    bool isArray() const {
        return _value.type() == typeid(ArrayType);
    }

    bool isObject() const {
        return _value.type() == typeid(ObjectType);
    }

    bool isConvertibleTo(JSONType other) const;

    size_t size() const;

    bool empty() const {
        if (isNull() || isArray() || isObject()) {
            return size() == 0;
        } else {
            return false;
        }
    }

    bool operator!() const {
        return isNull();
    }

    void clear();

    void resize(size_t newSize);

    JSONValue& operator[](size_t index);

    JSONValue& operator[](int index);

    const JSONValue& operator[](size_t index) const;

    const JSONValue& operator[](int index) const;

    JSONValue get(size_t index, const JSONValue &defaultValue) const {
        const JSONValue *value = &((*this)[index]);
        return value == &nullSingleton() ? defaultValue : *value;
    }

    bool isValidIndex(size_t index) const {
        return index < size();
    }

    JSONValue& append(const JSONValue &value) {
        return (*this)[size()] = value;
    }

    JSONValue& append(JSONValue &&value) {
        return (*this)[size()] = std::move(value);
    }

    JSONValue& operator[](const char *key);

    JSONValue& operator[](const std::string &key) {
        return (*this)[key.c_str()];
    }

    const JSONValue& operator[](const char *key) const;

    const JSONValue& operator[](const std::string &key) const {
        return (*this)[key.c_str()];
    }

    JSONValue get(const char *key, const JSONValue &defaultValue) const;

    JSONValue get(const std::string &key, const JSONValue &defaultValue) const {
        return get(key.c_str(), defaultValue);
    }

    const JSONValue* find(const char *key) const;

    const JSONValue* find(const std::string &key) const {
        return find(key.c_str());
    }

    bool removeMember(const char *key, JSONValue *removed= nullptr);

    bool removeMember(const std::string &key, JSONValue *removed= nullptr) {
        return removeMember(key.c_str(), removed);
    }

    bool removeIndex(size_t index, JSONValue *removed= nullptr);

    bool isMember(const char *key) const {
        return find(key) != nullptr;
    }

    bool isMember(const std::string &key) const {
        return isMember(key.c_str());
    }

    StringVector getMemberNames() const;

    void setComment(const char *comment, CommentPlacement placement) {
        _comments[placement] = comment;
    }

    void setComment(const std::string &comment, CommentPlacement placement) {
        _comments[placement] = comment;
    }

    void setComment(std::string &&comment, CommentPlacement placement) {
        _comments[placement] = std::move(comment);
    }

    bool hasComment(CommentPlacement placement) const {
        return !_comments[placement].empty();
    }

    const std::string& getComment(CommentPlacement placement) const {
        return _comments[placement];
    }

//    std::string toStyledString() const;

    ConstArrayIterator arrayBegin() const {
        auto &array = boost::get<ArrayType>(_value);
        return array.cbegin();
    }

    ConstArrayIterator arrayEnd() const {
        auto &array = boost::get<ArrayType>(_value);
        return array.cend();
    }

    ArrayIterator arrayBegin() {
        auto &array = boost::get<ArrayType>(_value);
        return array.begin();
    }

    ArrayIterator arrayEnd() {
        auto &array = boost::get<ArrayType>(_value);
        return array.end();
    }

    ConstObjectIterator objectBegin() const {
        auto &object = boost::get<ObjectType>(_value);
        return object.cbegin();
    }

    ConstObjectIterator objectEnd() const {
        auto &object = boost::get<ObjectType>(_value);
        return object.cend();
    }

    ObjectIterator objectBegin() {
        auto &object = boost::get<ObjectType>(_value);
        return object.begin();
    }

    ObjectIterator objectEnd() {
        auto &object = boost::get<ObjectType>(_value);
        return object.end();
    }

    static const JSONValue& nullSingleton();
private:
    ValueType _value;
    std::array<std::string, COMMENT_COUNT> _comments;
};


inline bool operator<(const JSONValue &lhs, const JSONValue &rhs) {
    return lhs._value < rhs._value;
}

inline bool operator<=(const JSONValue &lhs, const JSONValue &rhs) {
    return !(rhs < lhs);
}

inline bool operator>(const JSONValue &lhs, const JSONValue &rhs) {
    return rhs < lhs;
}

inline bool operator>=(const JSONValue &lhs, const JSONValue &rhs) {
    return !(lhs < rhs);
}

inline bool operator==(const JSONValue &lhs, const JSONValue &rhs) {
    return lhs._value == rhs._value;
}

inline bool operator!=(const JSONValue &lhs, const JSONValue &rhs) {
    return !(lhs == rhs);
}

inline bool operator<(const JSONValue::NullValue &lhs, const JSONValue::NullValue &rhs) {
    return false;
}

inline bool operator<=(const JSONValue::NullValue &lhs, const JSONValue::NullValue &rhs) {
    return !(rhs < lhs);
}

inline bool operator>(const JSONValue::NullValue &lhs, const JSONValue::NullValue &rhs) {
    return rhs < lhs;
}

inline bool operator>=(const JSONValue::NullValue &lhs, const JSONValue::NullValue &rhs) {
    return !(lhs < rhs);
}

inline bool operator==(const JSONValue::NullValue &lhs, const JSONValue::NullValue &rhs) {
    return true;
}

inline bool operator!=(const JSONValue::NullValue &lhs, const JSONValue::NullValue &rhs) {
    return !(lhs == rhs);
}


class NET4CXX_COMMON_API StreamWriter {
public:
    virtual ~StreamWriter() = default;

    virtual int write(const JSONValue &root, std::ostream *sout) = 0;

    class NET4CXX_COMMON_API Factory {
    public:
        virtual ~Factory() = default;

        virtual StreamWriter* newStreamWriter() const = 0;
    };
protected:
    std::ostream *_sout{nullptr};
};


enum class CommentStyle {
    None,
    Most,
    All
};


class NET4CXX_COMMON_API BuiltStyledStreamWriter: public StreamWriter {
public:
    BuiltStyledStreamWriter(std::string indentation,
                            CommentStyle cs,
                            std::string colonSymbol,
                            std::string nullSymbol,
                            std::string endingLineFeedSymbol,
                            bool useSpecialFloats,
                            unsigned int precision);

    int write(const JSONValue &root, std::ostream *sout) override;
private:
    void writeValue(const JSONValue &value);

    void writeArrayValue(const JSONValue &value);

    bool isMultilineArray(const JSONValue &value);

    void pushValue(const std::string &value) {
        if (_addChildValues) {
            _childValues.push_back(value);
        } else {
            *_sout << value;
        }
    }

    void writeIndent() {
        if (!_indentString.empty()) {
            *_sout << '\n' << _indentString;
        }
    }

    void writeWithIndent(const std::string &value) {
        if (!_indented) {
            writeIndent();
        }
        *_sout << value;
        _indented = false;
    }

    void indent() {
        _indentString += _indentation;
    }

    void unindent() {
        BOOST_ASSERT(_indentString.size() >= _indentation.size());
        _indentString.resize(_indentString.size() - _indentation.size());
    }

    void writeCommentBeforeValue(const JSONValue &root);

    void writeCommentAfterValueOnSameLine(const JSONValue &root);

    static bool hasCommentForValue(const JSONValue &value);

    StringVector _childValues;
    std::string _indentString;
    unsigned int _rightMargin;
    std::string _indentation;
    CommentStyle _cs;
    std::string _colonSymbol;
    std::string _nullSymbol;
    std::string _endingLineFeedSymbol;
    bool _addChildValues;
    bool _indented;
    bool _useSpecialFloats;
    unsigned int _precision;
};


class NET4CXX_COMMON_API StreamWriterBuilder: public StreamWriter::Factory {
public:
    StreamWriterBuilder();

    virtual StreamWriter* newStreamWriter() const override;

    JSONValue& settings() {
        return _settings;
    }

    const JSONValue& settings() const {
        return _settings;
    }

    bool validate(JSONValue *invalid= nullptr) const;

    JSONValue& operator[](const std::string &key) {
        return _settings[key];
    }

    static void setDefaults(JSONValue *settings);
protected:
    JSONValue _settings;
};


NET4CXX_COMMON_API std::string writeString(const StreamWriter::Factory &factory, const JSONValue &root);

NET4CXX_COMMON_API std::ostream& operator<<(std::ostream &sout, const JSONValue &root);

NS_END

#endif //NET4CXX_COMMON_CONFIGURATION_JSON_H
