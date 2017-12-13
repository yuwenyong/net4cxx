//
// Created by yuwenyong on 17-12-4.
//

#include "net4cxx/common/configuration/json.h"
#include "net4cxx/common/utilities/errors.h"


NS_BEGIN

int JSONValue::AsIntVisitor::operator()(long v) const {
    if (v <= INT_MIN || v >= INT_MAX) {
        NET4CXX_THROW_EXCEPTION(ValueError, "long out of int range");
    }
    return static_cast<int>(v);
}

int JSONValue::AsIntVisitor::operator()(double v) const {
    if (v <= INT_MIN || v >= INT_MAX) {
        NET4CXX_THROW_EXCEPTION(ValueError, "double out of int range");
    }
    return static_cast<int>(v);
}

int JSONValue::AsIntVisitor::operator()(const std::string &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to int");
}

int JSONValue::AsIntVisitor::operator()(const ArrayType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to int");
}

int JSONValue::AsIntVisitor::operator()(const ObjectType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to int");
}


long JSONValue::AsLongVisitor::operator()(double v) const {
    if (v <= LONG_MIN || v >= LONG_MAX) {
        NET4CXX_THROW_EXCEPTION(ValueError, "double out of long range");
    }
    return static_cast<long>(v);
}

long JSONValue::AsLongVisitor::operator()(const std::string &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to long");
}

long JSONValue::AsLongVisitor::operator()(const ArrayType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to long");
}

long JSONValue::AsLongVisitor::operator()(const ObjectType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to long");
}


float JSONValue::AsFloatVisitor::operator()(const std::string &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to float");
}

float JSONValue::AsFloatVisitor::operator()(const ArrayType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to float");
}

float JSONValue::AsFloatVisitor::operator()(const ObjectType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to float");
}


double JSONValue::AsDoubleVisitor::operator()(const std::string &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to double");
}

double JSONValue::AsDoubleVisitor::operator()(const ArrayType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to double");
}

double JSONValue::AsDoubleVisitor::operator()(const ObjectType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to double");
}


bool JSONValue::AsBoolVisitor::operator()(const std::string &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "string is not convertible to bool");
}

bool JSONValue::AsBoolVisitor::operator()(const ArrayType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to bool");
}

bool JSONValue::AsBoolVisitor::operator()(const ObjectType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to bool");
}


std::string JSONValue::AsStringVisitor::operator()(const ArrayType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "array is not convertible to string");
}

std::string JSONValue::AsStringVisitor::operator()(const ObjectType &v) const {
    NET4CXX_THROW_EXCEPTION(ValueError, "object is not convertible to string");
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

bool JSONValue::isConvertibleTo(JSONType other) const {
    switch (other) {
        case JSONType::JSON_NULL: {
            return (isNumeric() && asDouble() == 0.0)
                   || (_value.type() == typeid(bool) && !boost::get<bool>(_value))
                   || (_value.type() == typeid(std::string) && boost::get<std::string>(_value).empty())
                   || (_value.type() == typeid(ArrayType) && boost::get<ArrayType>(_value).empty())
                   || (_value.type() == typeid(ObjectType) && boost::get<ObjectType>(_value).empty())
                   || (_value.type() == typeid(NullValue));
        }
        case JSONType::JSON_INTEGER: {
            return isInteger() || _value.type() == typeid(bool) || _value.type() == typeid(NullValue);
        }
        case JSONType::JSON_REAL: {
            return isNumeric() || _value.type() == typeid(bool) || _value.type() == typeid(NullValue);
        }
        case JSONType::JSON_STRING: {
            return isNumeric() || _value.type() == typeid(bool) || _value.type() == typeid(std::string)
                   || _value.type() == typeid(NullValue);
        }
        case JSONType::JSON_BOOL: {
            return isNumeric() || _value.type() == typeid(bool) || _value.type() == typeid(NullValue);
        }
        case JSONType::JSON_ARRAY: {
            return _value.type() == typeid(ArrayType) || _value.type() == typeid(NullValue);
        }
        case JSONType::JSON_OBJECT: {
            return _value.type() == typeid(ObjectType) || _value.type() == typeid(NullValue);
        }
    }
    return false;
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
        setArray();
    }
    if (_value.type() != typeid(ArrayType)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "resize requires array value");
    }
    boost::get<ArrayType>(_value).resize(newSize);
}

JSONValue& JSONValue::operator[](size_t index) {
    if (_value.type() == typeid(NullValue)) {
        setArray();
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
        setObject();
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

NS_END