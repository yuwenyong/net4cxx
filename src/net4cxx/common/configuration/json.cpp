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


std::string JSONValue::valueToString(double value, bool useSpecialFloats, unsigned int precision) {
    return std::to_string(value);
}

NS_END