//
// Created by yuwenyong on 17-12-4.
//

#include "net4cxx/common/configuration/json.h"
#include "net4cxx/common/utilities/errors.h"


NS_BEGIN


static const double maxUInt64AsDouble = 18446744073709551615.0;


template <typename T, typename U>
static inline bool InRange(double d, T min, U max) {
    return d >= min && d <= max;
}

static std::string valueToString(double value, bool useSpecialFloats, unsigned int precision) {
    if (std::isfinite(value)) {
        std::ostringstream os;
        os << std::setprecision(precision) << value;
        return os.str();
    } else {
        if (value != value) {
            return useSpecialFloats ? "NaN" : "null";
        } else if (value < 0.0) {
            return useSpecialFloats ? "-Infinity" : "-1e+9999";
        } else {
            return useSpecialFloats ? "Infinity" : "1e+9999";
        }
    }
}

static std::string valueToString(double value) {
    return valueToString(value, false, 17);
}


static bool isAnyCharRequiredQuoting(const std::string &s) {
    for (auto c: s) {
        if (c == '\\' || c == '\"' || c < ' ' || static_cast<unsigned char>(c) < 0x80) {
            return true;
        }
    }
    return false;
}

static unsigned int utf8ToCodePoint(const char *&s, const char *e) {
    const unsigned int REPLACEMENT_CHARACTER = 0xFFFD;
    unsigned int firstByte = static_cast<unsigned char>(*s);
    if (firstByte < 0x80) {
        return firstByte;
    }
    if (firstByte < 0xE0) {
        if (e - s < 2) {
            return REPLACEMENT_CHARACTER;
        }
        unsigned int calculated = ((firstByte & 0x1F) << 6) | (static_cast<unsigned int>(s[1]) & 0x3F);
        s += 1;
        return calculated < 0x80 ? REPLACEMENT_CHARACTER : calculated;
    }

    if (firstByte < 0xF0) {
        if (e - s < 3) {
            return REPLACEMENT_CHARACTER;
        }
        unsigned int calculated = ((firstByte & 0x0F) << 12)
                                  | ((static_cast<unsigned int>(s[1]) & 0x3F) << 6)
                                  |  (static_cast<unsigned int>(s[2]) & 0x3F);
        s += 2;
        if (calculated >= 0xD800 && calculated >= 0xDFFF) {
            return REPLACEMENT_CHARACTER;
        }
        return calculated < 0x800 ? REPLACEMENT_CHARACTER : calculated;
    }

    if (firstByte < 0xF8) {
        if (e - s < 4) {
            return REPLACEMENT_CHARACTER;
        }
        unsigned int calculated = ((firstByte & 0x07) << 24)
                                  | ((static_cast<unsigned int>(s[1]) & 0x3F) << 12)
                                  | ((static_cast<unsigned int>(s[2]) & 0x3F) << 6)
                                  |  (static_cast<unsigned int>(s[3]) & 0x3F);
        s += 3;
        return calculated < 0x10000 ? REPLACEMENT_CHARACTER : calculated;
    }
    return REPLACEMENT_CHARACTER;
}


static const char hex2[] =
    "000102030405060708090a0b0c0d0e0f"
    "101112131415161718191a1b1c1d1e1f"
    "202122232425262728292a2b2c2d2e2f"
    "303132333435363738393a3b3c3d3e3f"
    "404142434445464748494a4b4c4d4e4f"
    "505152535455565758595a5b5c5d5e5f"
    "606162636465666768696a6b6c6d6e6f"
    "707172737475767778797a7b7c7d7e7f"
    "808182838485868788898a8b8c8d8e8f"
    "909192939495969798999a9b9c9d9e9f"
    "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
    "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
    "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
    "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
    "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
    "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

static std::string toHex16Bit(unsigned int x) {
    const unsigned int hi = (x >> 8) & 0xff;
    const unsigned int lo = x & 0xff;
    std::string result(4, ' ');
    result[0] = hex2[2 * hi];
    result[1] = hex2[2 * hi + 1];
    result[2] = hex2[2 * lo];
    result[3] = hex2[2 * lo + 1];
    return result;
}

static std::string valueToQuotedString(const std::string &value) {
    if (!isAnyCharRequiredQuoting(value)) {
        return "\"" + value + "\"";
    }
    std::string::size_type maxsize = value.size() * 2 + 3;
    std::string result;
    result.reserve(maxsize);
    result += "\"";
    const char *end = value.c_str() + value.size();
    for (const char *c = value.c_str(); c != end; ++c) {
        switch (*c) {
            case '\"': {
                result += "\\\"";
                break;
            }
            case '\\': {
                result += "\\\\";
                break;
            }
            case '\b': {
                result += "\\b";
                break;
            }
            case '\f': {
                result += "\\f";
                break;
            }
            case '\n': {
                result += "\\n";
                break;
            }
            case '\r': {
                result += "\\r";
                break;
            }
            case '\t': {
                result += "\\t";
                break;
            }
            default: {
                unsigned int cp = utf8ToCodePoint(c, end);
                if (cp < 0x80 && cp >= 0x20) {
                    result += static_cast<char>(cp);
                }
                else if (cp < 0x10000) {
                    result += "\\u";
                    result += toHex16Bit(cp);
                }
                else {
                    cp -= 0x10000;
                    result += "\\u";
                    result += toHex16Bit((cp >> 10) + 0xD800);
                    result += "\\u";
                    result += toHex16Bit((cp & 0x3FF) + 0xDC00);
                }
                break;
            }
        }
    }
    result += "\"";
    return result;
}


JSONValue::JSONValue(JSONType type) {
    switch (type) {
        case JSONType::nullValue: {
            break;
        }
        case JSONType::intValue: {
            _value = 0L;
            break;
        }
        case JSONType::uintValue: {
            _value = 0UL;
            break;
        }
        case JSONType::realValue: {
            _value = 0.0;
            break;
        }
        case JSONType::stringValue: {
            _value = "";
            break;
        }
        case JSONType::boolValue: {
            _value = false;
            break;
        }
        case JSONType::arrayValue: {
            _value = ArrayType{};
            break;
        }
        case JSONType::objectValue: {
            _value = ObjectType{};
            break;
        }
    }
}

JSONType JSONValue::type() const {

    struct GetTypeVisitor: public boost::static_visitor<JSONType> {
        result_type operator()(NullValue v) const {
            return JSONType::nullValue;
        }

        result_type operator()(int64_t v) const {
            return JSONType::intValue;
        }

        result_type operator()(uint64_t v) const {
            return JSONType::uintValue;
        }

        result_type operator()(double v) const {
            return JSONType::realValue;
        }

        result_type operator()(const std::string &v) const {
            return JSONType::stringValue;
        }

        result_type operator()(bool v) const {
            return JSONType::boolValue;
        }

        result_type operator()(const ArrayType &v) const {
            return JSONType::arrayValue;
        }

        result_type operator()(const ObjectType &v) const {
            return JSONType::objectValue;
        }
    };

    return boost::apply_visitor(GetTypeVisitor(), _value);
}

int JSONValue::compare(const JSONValue &other) const {
    if (*this < other) {
        return -1;
    }
    if (*this > other) {
        return 1;
    }
    return 0;
}

std::string JSONValue::asString() const {

    struct AsStringVisitor: public boost::static_visitor<std::string> {
        result_type operator()(NullValue v) const {
            return "";
        }

        result_type operator()(int64_t v) const {
            return std::to_string(v);
        }

        result_type operator()(uint64_t v) const {
            return std::to_string(v);
        }

        result_type operator()(double v) const {
            return valueToString(v);
        }

        result_type operator()(const std::string &v) const {
            return v;
        }

        result_type operator()(bool v) const {
            return v ? "true" : "false";
        }

        result_type operator()(const ArrayType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to string");
        }

        result_type operator()(const ObjectType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to string");
        }
    };

    return boost::apply_visitor(AsStringVisitor(), _value);
}

int JSONValue::asInt() const {

    struct AsIntVisitor: public boost::static_visitor<int> {
        result_type operator()(NullValue v) const {
            return 0;
        }

        result_type operator()(int64_t v) const {
            if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max()) {
                NET4CXX_THROW_EXCEPTION(ValueError, "int64 out of int range");
            }
            return static_cast<int>(v);
        }

        result_type operator()(uint64_t v) const {
            if (v > std::numeric_limits<int>::max()) {
                NET4CXX_THROW_EXCEPTION(ValueError, "uint64 out of int range");
            }
            return static_cast<int>(v);
        }

        result_type operator()(double v) const {
            if (!InRange(v, std::numeric_limits<int>::min(), std::numeric_limits<int>::max())) {
                NET4CXX_THROW_EXCEPTION(ValueError, "double out of int range");
            }
            return static_cast<int>(v);
        }

        result_type operator()(const std::string &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to int");
        }

        result_type operator()(bool v) const {
            return v ? 1 : 0;
        }

        result_type operator()(const ArrayType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to int");
        }

        result_type operator()(const ObjectType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to int");
        }
    };

    return boost::apply_visitor(AsIntVisitor(), _value);
}

unsigned int JSONValue::asUInt() const {

    struct AsUIntVisitor: public boost::static_visitor<unsigned int> {
        result_type operator()(NullValue v) const {
            return 0;
        }

        result_type operator()(int64_t v) const {
            if (v < 0 || v > std::numeric_limits<unsigned int>::max()) {
                NET4CXX_THROW_EXCEPTION(ValueError, "int64 out of uint range");
            }
            return static_cast<unsigned int>(v);
        }

        result_type operator()(uint64_t v) const {
            if (v > std::numeric_limits<unsigned int>::max()) {
                NET4CXX_THROW_EXCEPTION(ValueError, "uint64 out of uint range");
            }
            return static_cast<unsigned int>(v);
        }

        result_type operator()(double v) const {
            if (!InRange(v, 0, std::numeric_limits<unsigned int>::max())) {
                NET4CXX_THROW_EXCEPTION(ValueError, "double out of uint range");
            }
            return static_cast<unsigned int>(v);
        }

        result_type operator()(const std::string &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to uint");
        }

        result_type operator()(bool v) const {
            return v ? 1 : 0;
        }

        result_type operator()(const ArrayType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to uint");
        }

        result_type operator()(const ObjectType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to uint");
        }
    };

    return boost::apply_visitor(AsUIntVisitor(), _value);
}

int64_t JSONValue::asInt64() const {

    struct AsInt64Visitor: public boost::static_visitor<int64_t> {
        result_type operator()(NullValue v) const {
            return 0;
        }

        result_type operator()(int64_t v) const {
            return v;
        }

        result_type operator()(uint64_t v) const {
            if (v > std::numeric_limits<int64_t>::max()) {
                NET4CXX_THROW_EXCEPTION(ValueError, "uint64 out of int64 range");
            }
            return static_cast<int64_t>(v);
        }

        result_type operator()(double v) const {
            if (!InRange(v, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max())) {
                NET4CXX_THROW_EXCEPTION(ValueError, "double out of int64 range");
            }
            return static_cast<int64_t>(v);
        }

        result_type operator()(const std::string &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to int64");
        }

        result_type operator()(bool v) const {
            return v ? 1 : 0;
        }

        result_type operator()(const ArrayType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to int64");
        }

        result_type operator()(const ObjectType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to int64");
        }
    };

    return boost::apply_visitor(AsInt64Visitor(), _value);
}

uint64_t JSONValue::asUInt64() const {

    struct AsUInt64Visitor: public boost::static_visitor<uint64_t> {
        result_type operator()(NullValue v) const {
            return 0;
        }

        result_type operator()(int64_t v) const {
            if (v < 0) {
                NET4CXX_THROW_EXCEPTION(ValueError, "int64 out of uint64 range");
            }
            return static_cast<uint64_t>(v);
        }

        result_type operator()(uint64_t v) const {
            return v;
        }

        result_type operator()(double v) const {
            if (!InRange(v, 0, std::numeric_limits<uint64_t>::max())) {
                NET4CXX_THROW_EXCEPTION(ValueError, "double out of uint64 range");
            }
            return static_cast<uint64_t>(v);
        }

        result_type operator()(const std::string &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to uint64");
        }

        result_type operator()(bool v) const {
            return v ? 1 : 0;
        }

        result_type operator()(const ArrayType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to uint64");
        }

        result_type operator()(const ObjectType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to uint64");
        }
    };

    return boost::apply_visitor(AsUInt64Visitor(), _value);
}

float JSONValue::asFloat() const {

    struct AsFloatVisitor: public boost::static_visitor<float> {
        result_type operator()(NullValue v) const {
            return 0.0f;
        }

        result_type operator()(int64_t v) const {
            return static_cast<float>(v);
        }

        result_type operator()(uint64_t v) const {
            return static_cast<float>(v);
        }

        result_type operator()(double v) const {
            return static_cast<float>(v);
        }

        result_type operator()(const std::string &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to float");
        }

        result_type operator()(bool v) const {
            return v ? 1.0f : 0.0f;
        }

        result_type operator()(const ArrayType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to float");
        }

        result_type operator()(const ObjectType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to float");
        }
    };

    return boost::apply_visitor(AsFloatVisitor(), _value);
}

double JSONValue::asDouble() const {

    struct AsDoubleVisitor: public boost::static_visitor<double> {
        result_type operator()(NullValue v) const {
            return 0.0;
        }

        result_type operator()(int64_t v) const {
            return static_cast<double>(v);
        }

        result_type operator()(uint64_t v) const {
            return static_cast<double>(v);
        }

        result_type operator()(double v) const {
            return v;
        }

        result_type operator()(const std::string &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to double");
        }

        result_type operator()(bool v) const {
            return v ? 1.0 : 0.0;
        }

        result_type operator()(const ArrayType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to double");
        }

        result_type operator()(const ObjectType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to double");
        }
    };

    return boost::apply_visitor(AsDoubleVisitor(), _value);
}

bool JSONValue::asBool() const {

    struct AsBoolVisitor: public boost::static_visitor<bool> {
        result_type operator()(NullValue v) const {
            return false;
        }

        result_type operator()(int64_t v) const {
            return v != 0;
        }

        result_type operator()(uint64_t v) const {
            return v != 0;
        }

        result_type operator()(double v) const {
            return v != 0.0;
        }

        result_type operator()(const std::string &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to bool");
        }

        result_type operator()(bool v) const {
            return v;
        }

        result_type operator()(const ArrayType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to bool");
        }

        result_type operator()(const ObjectType &v) const {
            NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to bool");
        }
    };

    return boost::apply_visitor(AsBoolVisitor(), _value);
}

bool JSONValue::isInt() const {

    struct IsIntVisitor: public boost::static_visitor<bool> {
        result_type operator()(NullValue v) const {
            return false;
        }

        result_type operator()(int64_t v) const {
            return v >= std::numeric_limits<int>::min() && v <= std::numeric_limits<int>::max();
        }

        result_type operator()(uint64_t v) const {
            return v <= std::numeric_limits<int>::max();
        }

        result_type operator()(double v) const {
            return v >= std::numeric_limits<int>::min() && v <= std::numeric_limits<int>::max() && IsIntegral(v);
        }

        result_type operator()(const std::string &v) const {
            return false;
        }

        result_type operator()(bool v) const {
            return false;
        }

        result_type operator()(const ArrayType &v) const {
            return false;
        }

        result_type operator()(const ObjectType &v) const {
            return false;
        }
    };

    return boost::apply_visitor(IsIntVisitor(), _value);
}

bool JSONValue::isInt64() const {

    struct IsInt64Visitor: public boost::static_visitor<bool> {
        result_type operator()(NullValue v) const {
            return false;
        }

        result_type operator()(int64_t v) const {
            return true;
        }

        result_type operator()(uint64_t v) const {
            return v <= std::numeric_limits<int64_t>::max();
        }

        result_type operator()(double v) const {
            return v >= std::numeric_limits<int64_t>::min() && v < std::numeric_limits<int64_t>::max() && IsIntegral(v);
        }

        result_type operator()(const std::string &v) const {
            return false;
        }

        result_type operator()(bool v) const {
            return false;
        }

        result_type operator()(const ArrayType &v) const {
            return false;
        }

        result_type operator()(const ObjectType &v) const {
            return false;
        }
    };

    return boost::apply_visitor(IsInt64Visitor(), _value);
}

bool JSONValue::isUInt() const {

    struct IsUIntVisitor: public boost::static_visitor<bool> {
        result_type operator()(NullValue v) const {
            return false;
        }

        result_type operator()(int64_t v) const {
            return v >= 0 && v <= std::numeric_limits<unsigned int>::max();
        }

        result_type operator()(uint64_t v) const {
            return v <= std::numeric_limits<unsigned int>::max();
        }

        result_type operator()(double v) const {
            return v >= 0 && v <= std::numeric_limits<unsigned int>::max() && IsIntegral(v);
        }

        result_type operator()(const std::string &v) const {
            return false;
        }

        result_type operator()(bool v) const {
            return false;
        }

        result_type operator()(const ArrayType &v) const {
            return false;
        }

        result_type operator()(const ObjectType &v) const {
            return false;
        }
    };

    return boost::apply_visitor(IsUIntVisitor(), _value);
}

bool JSONValue::isUInt64() const {

    struct IsUInt64Visitor: public boost::static_visitor<bool> {
        result_type operator()(NullValue v) const {
            return false;
        }

        result_type operator()(int64_t v) const {
            return v >= 0;
        }

        result_type operator()(uint64_t v) const {
            return true;
        }

        result_type operator()(double v) const {
            return v >= 0 && v < maxUInt64AsDouble && IsIntegral(v);
        }

        result_type operator()(const std::string &v) const {
            return false;
        }

        result_type operator()(bool v) const {
            return false;
        }

        result_type operator()(const ArrayType &v) const {
            return false;
        }

        result_type operator()(const ObjectType &v) const {
            return false;
        }
    };

    return boost::apply_visitor(IsUInt64Visitor(), _value);
}

bool JSONValue::isIntegral() const {

    struct IsIntegralVisitor: public boost::static_visitor<bool> {
        result_type operator()(NullValue v) const {
            return false;
        }

        result_type operator()(int64_t v) const {
            return true;
        }

        result_type operator()(uint64_t v) const {
            return true;
        }

        result_type operator()(double v) const {
            return v >= std::numeric_limits<int64_t>::min()  && v < maxUInt64AsDouble && IsIntegral(v);
        }

        result_type operator()(const std::string &v) const {
            return false;
        }

        result_type operator()(bool v) const {
            return false;
        }

        result_type operator()(const ArrayType &v) const {
            return false;
        }

        result_type operator()(const ObjectType &v) const {
            return false;
        }
    };

    return boost::apply_visitor(IsIntegralVisitor(), _value);
}

bool JSONValue::isConvertibleTo(JSONType other) const {
    switch (other) {
        case JSONType::nullValue: {
            return (isNumeric() && asDouble() == 0.0)
                   || (_value.type() == typeid(bool) && !boost::get<bool>(_value))
                   || (_value.type() == typeid(std::string) && boost::get<std::string>(_value).empty())
                   || (_value.type() == typeid(ArrayType) && boost::get<ArrayType>(_value).empty())
                   || (_value.type() == typeid(ObjectType) && boost::get<ObjectType>(_value).empty())
                   || (_value.type() == typeid(NullValue));
        }
        case JSONType::intValue: {
            return isInt()
                   || (_value.type() == typeid(double)
                       && InRange(boost::get<double>(_value), std::numeric_limits<int>::min(),
                                  std::numeric_limits<int>::max()))
                   || _value.type() == typeid(bool)
                   || _value.type() == typeid(NullValue);
        }
        case JSONType::uintValue: {
            return isUInt()
                   || (_value.type() == typeid(double)
                       && InRange(boost::get<double>(_value), 0, std::numeric_limits<unsigned int>::max()))
                   || _value.type() == typeid(bool)
                   || _value.type() == typeid(NullValue);
        }
        case JSONType::realValue: {
            return isNumeric() || _value.type() == typeid(bool) || _value.type() == typeid(NullValue);
        }
        case JSONType::stringValue: {
            return isNumeric() || _value.type() == typeid(bool) || _value.type() == typeid(std::string)
                   || _value.type() == typeid(NullValue);
        }
        case JSONType::boolValue: {
            return isNumeric() || _value.type() == typeid(bool) || _value.type() == typeid(NullValue);
        }
        case JSONType::arrayValue: {
            return _value.type() == typeid(ArrayType) || _value.type() == typeid(NullValue);
        }
        case JSONType::objectValue: {
            return _value.type() == typeid(ObjectType) || _value.type() == typeid(NullValue);
        }
    }
    BOOST_ASSERT(false);
    return false; // unreachable
}

size_t JSONValue::size() const {
    if (_value.type() == typeid(ArrayType)) {
        return boost::get<ArrayType>(_value).size();
    } else if (_value.type() == typeid(ObjectType)) {
        return boost::get<ObjectType>(_value).size();
    } else {
        return 0;
    }
}

void JSONValue::clear() {
    if (_value.type() == typeid(ArrayType)) {
        boost::get<ArrayType>(_value).clear();
    } else if (_value.type() == typeid(ObjectType)) {
        boost::get<ObjectType>(_value).clear();
    } else if (_value.type() == typeid(NullValue)) {

    } else {
        NET4CXX_THROW_EXCEPTION(ValueError, "clear requires complex value");
    }
}

void JSONValue::resize(size_t newSize) {
    if (_value.type() == typeid(NullValue)) {
        *this = JSONValue(JSONType::arrayValue);
    }
    if (_value.type() != typeid(ArrayType)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "resize requires array value");
    }
    boost::get<ArrayType>(_value).resize(newSize);
}

JSONValue& JSONValue::operator[](size_t index) {
    if (_value.type() == typeid(NullValue)) {
        *this = JSONValue(JSONType::arrayValue);
    } else if (_value.type() != typeid(ArrayType)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "operator[](index) requires array value");
    }
    auto &array = boost::get<ArrayType>(_value);
    if (index >= array.size()) {
        array.resize(index + 1);
    }
    return array[index];
}

JSONValue& JSONValue::operator[](int index) {
    if (index < 0) {
        NET4CXX_THROW_EXCEPTION(ValueError, "index cannot be negative");
    }
    return (*this)[static_cast<size_t>(index)];
}

const JSONValue& JSONValue::operator[](size_t index) const {
    if (_value.type() == typeid(NullValue)) {
        return nullSingleton();
    } else if (_value.type() == typeid(ArrayType)) {
        auto &array = boost::get<ArrayType>(_value);
        if (index < array.size()) {
            return array[index];
        } else {
            return nullSingleton();
        }
    } else {
        NET4CXX_THROW_EXCEPTION(ValueError, "operator[](index)const requires array value");
    }
}

const JSONValue& JSONValue::operator[](int index) const {
    if (index < 0) {
        NET4CXX_THROW_EXCEPTION(ValueError, "index cannot be negative");
    }
    return (*this)[static_cast<size_t>(index)];
}

JSONValue& JSONValue::operator[](const char *key) {
    if (_value.type() == typeid(NullValue)) {
        *this = JSONValue(JSONType::objectValue);
    }else if (_value.type() != typeid(ObjectType)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "operator[](key) requires object value");
    }
    auto &object = boost::get<ObjectType>(_value);
    auto it = object.find(key);
    if (it == object.end()) {
        it = object.insert(it, std::make_pair(key, nullSingleton()));
    }
    return it->second;
}

const JSONValue& JSONValue::operator[](const char *key) const {
    const JSONValue *found = find(key);
    return found ? *found : nullSingleton();
}

JSONValue JSONValue::get(const char *key, const JSONValue &defaultValue) const {
    const JSONValue *found = find(key);
    return found ? *found : defaultValue;
}

const JSONValue* JSONValue::find(const char *key) const {
    if (_value.type() == typeid(NullValue)) {
        return nullptr;
    } else if (_value.type() == typeid(ObjectType)) {
        auto &object = boost::get<ObjectType>(_value);
        auto it = object.find(key);
        return it != object.end() ? &(it->second) : nullptr;
    } else {
        NET4CXX_THROW_EXCEPTION(ValueError, "find requires object value or null value");
    }
}

bool JSONValue::removeMember(const char *key, JSONValue *removed) {
    if (_value.type() != typeid(ObjectType)) {
        return false;
    }
    auto &object = boost::get<ObjectType>(_value);
    auto it = object.find(key);
    if (it == object.end()) {
        return false;
    }
    if (removed) {
        *removed = std::move(it->second);
    }
    object.erase(it);
    return true;
}

bool JSONValue::removeIndex(size_t index, JSONValue *removed) {
    if (_value.type() != typeid(ArrayType)) {
        return false;
    }
    auto &array = boost::get<ArrayType>(_value);
    if (index >= array.size()) {
        return false;
    }
    if (removed) {
        *removed = std::move(array[index]);
    }
    array.erase(std::next(array.begin(), index));
    return true;
}

StringVector JSONValue::getMemberNames() const {
    StringVector members;
    if (_value.type() == typeid(NullValue)) {

    } else if (_value.type() == typeid(ObjectType)) {
        auto &object = boost::get<ObjectType>(_value);
        members.reserve(object.size());
        for (auto &kv: object) {
            members.push_back(kv.first);
        }
    } else {
        NET4CXX_THROW_EXCEPTION(ValueError, "getMemberNames requires object value");
    }
    return members;
}

const JSONValue& JSONValue::nullSingleton() {
    static const JSONValue nullStatic;
    return nullStatic;
}


BuiltStyledStreamWriter::BuiltStyledStreamWriter(
        std::string indentation,
        CommentStyle cs,
        std::string colonSymbol,
        std::string nullSymbol,
        std::string endingLineFeedSymbol,
        bool useSpecialFloats,
        unsigned int precision)
        : _rightMargin(74)
        , _indentation(std::move(indentation))
        , _cs(cs)
        , _colonSymbol(std::move(colonSymbol))
        , _nullSymbol(std::move(nullSymbol))
        , _endingLineFeedSymbol(std::move(endingLineFeedSymbol))
        , _addChildValues(false)
        , _indented(false)
        , _useSpecialFloats(useSpecialFloats)
        , _precision(precision) {

}

int BuiltStyledStreamWriter::write(const JSONValue &root, std::ostream *sout) {
    _sout = sout;
    _addChildValues = false;
    _indented = true;
    _indentString.clear();
    writeCommentBeforeValue(root);
    if (!_indented) {
        writeIndent();
    }
    _indented = true;
    writeValue(root);
    writeCommentAfterValueOnSameLine(root);
    *_sout << _endingLineFeedSymbol;
    _sout = nullptr;
    return 0;
}

void BuiltStyledStreamWriter::writeValue(const JSONValue &value) {
    switch (value.type()) {
        case JSONType::nullValue: {
            pushValue(_nullSymbol);
            break;
        }
        case JSONType::intValue: {
            pushValue(std::to_string(value.asInt64()));
            break;
        }
        case JSONType::uintValue: {
            pushValue(std::to_string(value.asUInt64()));
            break;
        }
        case JSONType::realValue: {
            pushValue(valueToString(value.asDouble(), _useSpecialFloats, _precision));
            break;
        }
        case JSONType::stringValue: {
            pushValue(valueToQuotedString(value.asString()));
            break;
        }
        case JSONType::boolValue: {
            pushValue(value.asBool() ? "true" : "false");
            break;
        }
        case JSONType::arrayValue: {
            writeArrayValue(value);
            break;
        }
        case JSONType::objectValue: {
            StringVector members = value.getMemberNames();
            if (members.empty()) {
                pushValue("{}");
            } else {
                writeWithIndent("{");
                indent();
                for (auto it = members.begin();it != members.end(); ++ it) {
                    const std::string &name = *it;
                    const JSONValue &childValue = value[name];
                    writeCommentBeforeValue(childValue);
                    writeWithIndent(valueToQuotedString(name));
                    *_sout << _colonSymbol;
                    writeValue(childValue);
                    if (std::next(it) != members.end()) {
                        *_sout << ",";
                    }
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("}");
            }
            break;
        }
    }
}

void BuiltStyledStreamWriter::writeArrayValue(const JSONValue &value) {
    size_t size = value.size();
    if (size == 0) {
        pushValue("[]");
    } else {
        bool isMultiLine = (_cs == CommentStyle::All) || isMultilineArray(value);
        if (isMultiLine) {
            writeWithIndent("[");
            indent();
            bool hasChildValue = !_childValues.empty();
            for (size_t index = 0; index != size; ++index) {
                const JSONValue &childValue = value[index];
                writeCommentBeforeValue(childValue);
                if (hasChildValue) {
                    writeWithIndent(_childValues[index]);
                } else {
                    if (!_indented) {
                        writeIndent();
                    }
                    _indented = true;
                    writeValue(childValue);
                    _indented = false;
                }
                if (index + 1 != size) {
                    *_sout << ",";
                }
                writeCommentAfterValueOnSameLine(childValue);
            }
            unindent();
            writeWithIndent("]");
        } else {
            BOOST_ASSERT(_childValues.size() == size);
            *_sout << "[";
            if (!_indentation.empty()) {
                *_sout << " ";
            }
            for (unsigned index = 0; index < size; ++index) {
                if (index > 0) {
                    *_sout << ((!_indentation.empty()) ? ", " : ",");
                }
                *_sout << _childValues[index];
            }
            if (!_indentation.empty()) {
                *_sout << " ";
            }
            *_sout << "]";
        }
    }
}

bool BuiltStyledStreamWriter::isMultilineArray(const JSONValue &value) {
    size_t size = value.size();
    bool isMultiLine = size * 3 >= _rightMargin;
    _childValues.clear();
    for (size_t index = 0; index < size && !isMultiLine; ++index) {
        const JSONValue &childValue = value[index];
        isMultiLine = ((childValue.isArray() || childValue.isObject()) && !childValue.empty());
    }
    if (!isMultiLine) {
        _childValues.reserve(size);
        _addChildValues = true;
        size_t lineLength = 4 + (size - 1) * 2; // '[ ' + ', '*n + ' ]'
        for (size_t index = 0; index < size; ++index) {
            if (hasCommentForValue(value[index])) {
                isMultiLine = true;
            }
            writeValue(value[index]);
            lineLength += _childValues[index].length();
        }
        _addChildValues = false;
        isMultiLine = isMultiLine || lineLength >= _rightMargin;
    }
    return isMultiLine;
}

void BuiltStyledStreamWriter::writeCommentBeforeValue(const JSONValue &root) {
    if (_cs == CommentStyle::None) {
        return;
    }
    if (!root.hasComment(COMMENT_BEFORE)) {
        return;
    }
    if (!_indented) {
        writeIndent();
    }
    const std::string &comment = root.getComment(COMMENT_BEFORE);
    for (auto iter = comment.begin(); iter != comment.end(); ++iter) {
        *_sout << *iter;
        if (*iter == '\n' && (iter + 1) != comment.end() && *(iter + 1) == '/') {
            *_sout << _indentString;
        }
    }
    _indented = false;
}

void BuiltStyledStreamWriter::writeCommentAfterValueOnSameLine(const JSONValue &root) {
    if (_cs == CommentStyle::None) {
        return;
    }
    if (root.hasComment(COMMENT_ON_SAME_LINE)) {
        *_sout << " " << root.getComment(COMMENT_ON_SAME_LINE);
    }
    if (root.hasComment(COMMENT_AFTER)) {
        writeIndent();
        *_sout << root.getComment(COMMENT_AFTER);
    }
}

bool BuiltStyledStreamWriter::hasCommentForValue(const JSONValue &value) {
    return value.hasComment(COMMENT_BEFORE)
           || value.hasComment(COMMENT_ON_SAME_LINE)
           || value.hasComment(COMMENT_AFTER);
}


StreamWriterBuilder::StreamWriterBuilder() {
    setDefaults(&_settings);
}

StreamWriter* StreamWriterBuilder::newStreamWriter() const {
    std::string indentation = _settings["indentation"].asString();
    std::string cs_str = _settings["commentStyle"].asString();
    bool eyc = _settings["enableYAMLCompatibility"].asBool();
    bool dnp = _settings["dropNullPlaceholders"].asBool();
    bool usf = _settings["useSpecialFloats"].asBool();
    int pre = _settings["precision"].asInt();
    CommentStyle cs;
    if (cs_str == "All") {
        cs = CommentStyle::All;
    } else if (cs_str == "None") {
        cs = CommentStyle::None;
    } else {
        NET4CXX_THROW_EXCEPTION(ValueError, "commentStyle must be 'All' or 'None'");
    }
    std::string colonSymbol = " : ";
    if (eyc) {
        colonSymbol = ": ";
    } else if (indentation.empty()) {
        colonSymbol = ":";
    }
    std::string nullSymbol = "null";
    if (dnp) {
        nullSymbol.clear();
    }
    pre = std::min(pre, 17);
    std::string endingLineFeedSymbol;
    return new BuiltStyledStreamWriter(std::move(indentation), cs, std::move(colonSymbol), std::move(nullSymbol),
                                       std::move(endingLineFeedSymbol), usf, (unsigned int)pre);
}

static void getValidWriterKeys(StringSet *validKeys) {
    validKeys->clear();
    validKeys->insert("indentation");
    validKeys->insert("commentStyle");
    validKeys->insert("enableYAMLCompatibility");
    validKeys->insert("dropNullPlaceholders");
    validKeys->insert("useSpecialFloats");
    validKeys->insert("precision");
}

bool StreamWriterBuilder::validate(JSONValue *invalid) const {
    JSONValue myInvalid;
    if (!invalid) {
        invalid = &myInvalid;
    }
    JSONValue &inv = *invalid;
    StringSet validKeys;
    getValidWriterKeys(&validKeys);
    StringVector keys = _settings.getMemberNames();
    for (auto &key: keys) {
        if (validKeys.find(key) == validKeys.end()) {
            inv[key] = _settings[key];
        }
    }
    return inv.empty();
}

void StreamWriterBuilder::setDefaults(JSONValue *settings) {
    (*settings)["commentStyle"] = "All";
    (*settings)["indentation"] = "\t";
    (*settings)["enableYAMLCompatibility"] = false;
    (*settings)["dropNullPlaceholders"] = false;
    (*settings)["useSpecialFloats"] = false;
    (*settings)["precision"] = 17;
}


std::string writeString(const StreamWriter::Factory &factory, const JSONValue &root) {
    std::ostringstream sout;
    std::unique_ptr<StreamWriter> writer(factory.newStreamWriter());
    writer->write(root, &sout);
    return sout.str();
}

std::ostream& operator<<(std::ostream &sout, const JSONValue &root) {
    StreamWriterBuilder builder;
    std::unique_ptr<StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &sout);
    return sout;
}

NS_END