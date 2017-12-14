//
// Created by yuwenyong on 17-12-4.
//

#include "net4cxx/common/configuration/json.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/utilities/errors.h"


NS_BEGIN


static const double maxUInt64AsDouble = 18446744073709551615.0;


template <typename T, typename U>
static inline bool InRange(double d, T min, U max) {
    return d >= min && d <= max;
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

JSONType JSONValue::getType() const {

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
            return JSONType::realValue;
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

std::string JSONValue::valueToString(double value, bool useSpecialFloats, unsigned int precision) {
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


BuiltStyledStreamWriter::BuiltStyledStreamWriter(
        std::string indentation,
        CommentStyle cs,
        std::string colonSymbol,
        std::string nullSymbol,
        std::string endingLineFeedSymbol,
        bool useSpecialFloats,
        unsigned int precision)
        : _rightMargin(74)
        , _indentation_(std::move(indentation))
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
    return 0;
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