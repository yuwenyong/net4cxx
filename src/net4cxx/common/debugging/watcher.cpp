//
// Created by yuwenyong on 17-9-18.
//

#include "net4cxx/common/debugging/watcher.h"
#include <boost/functional/factory.hpp>
#include "net4cxx/common/debugging/assert.h"


NS_BEGIN

void Watcher::set(const char *key, int value) {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _objs.find(key);
    int oldValue;
    if (iter == _objs.end()) {
        oldValue = 0;
        _objs.emplace(key, value);
    } else {
        oldValue = iter->second;
        iter->second = value;
    }
    auto sig = _setCallbacks.find(key);
    if (sig != _setCallbacks.end()) {
        (*sig->second)(oldValue, value);
    }
}

void Watcher::inc(const char *key, int increment) {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _objs.find(key);
    int oldValue, newValue;
    if (iter == _objs.end()) {
        oldValue = 0;
        _objs.emplace(key, increment);
        newValue = increment;
    } else {
        oldValue = iter->second;
        iter->second += increment;
        newValue = iter->second;
    }
    auto sig = _incCallbacks.find(key);
    if (sig != _incCallbacks.end()) {
        (*sig->second)(oldValue, increment, newValue);
    }
}

void Watcher::dec(const char *key, int decrement) {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _objs.find(key);
    int oldValue, newValue;
    if (iter == _objs.end()) {
        oldValue = 0;
        _objs.emplace(key, -decrement);
        newValue = -decrement;
    } else {
        oldValue = iter->second;
        iter->second -= decrement;
        newValue = iter->second;
    }
    auto sig = _decCallbacks.find(key);
    if (sig != _decCallbacks.end()) {
        (*sig->second)(oldValue, decrement, newValue);
    }
}

void Watcher::del(const char *key) {
    std::lock_guard<std::mutex> lock(_lock);
    int value;
    auto iter = _objs.find(key);
    if (iter != _objs.end()) {
        value = iter->second;
        _objs.erase(iter);
        auto sig = _delCallbacks.find(key);
        if (sig != _delCallbacks.end()) {
            (*sig->second)(value);
        }
    }
}

void Watcher::addSetCallback(const char *key, const SetCallback &callback) {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _setCallbacks.find(key);
    SetCallbackSignal *sig = nullptr;
    if (iter == _setCallbacks.end()) {
        std::string keyName(key);
        sig = boost::factory<SetCallbackSignal *>()();
        _setCallbacks.insert(keyName, sig);
    } else {
        sig = iter->second;
    }
    NET4CXX_ASSERT(sig != nullptr);
    sig->connect(callback);
}

void Watcher::addIncCallback(const char *key, const IncCallback &callback) {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _incCallbacks.find(key);
    IncCallbackSignal *sig = nullptr;
    if (iter == _incCallbacks.end()) {
        std::string keyName(key);
        sig = boost::factory<IncCallbackSignal *>()();
        _incCallbacks.insert(keyName, sig);
    } else {
        sig = iter->second;
    }
    NET4CXX_ASSERT(sig != nullptr);
    sig->connect(callback);
}

void Watcher::addDecCallback(const char *key, const DecCallback &callback) {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _decCallbacks.find(key);
    DecCallbackSignal *sig = nullptr;
    if (iter == _decCallbacks.end()) {
        std::string keyName(key);
        sig = boost::factory<DecCallbackSignal *>()();
        _decCallbacks.insert(keyName, sig);
    } else {
        sig = iter->second;
    }
    NET4CXX_ASSERT(sig != nullptr);
    sig->connect(callback);
}

void Watcher::addDelCallback(const char *key, const DelCallback &callback) {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _delCallbacks.find(key);
    DelCallbackSignal *sig = nullptr;
    if (iter == _delCallbacks.end()) {
        std::string keyName(key);
        sig = boost::factory<DelCallbackSignal *>()();
        _delCallbacks.insert(keyName, sig);
    } else {
        sig = iter->second;
    }
    sig->connect(callback);
}

void Watcher::dump(Filter filter, Logger *logger) const {
    std::lock_guard<std::mutex> lock(_lock);
    if (_objs.empty()) {
        return;
    }
    dumpHeader(logger);
    for (const auto &object: _objs) {
        if (filter(object.first, object.second)) {
            dumpObject(object.first,  object.second, logger);
        }
    }
    dumpFooter(logger);
}

void Watcher::dumpAll(Logger *logger) const {
    std::lock_guard<std::mutex> lock(_lock);
    if (_objs.empty()) {
        return;
    }
    dumpHeader(logger);
    for (const auto &object: _objs) {
        dumpObject(object.first,  object.second, logger);
    }
    dumpFooter(logger);
}

void Watcher::dumpNonZero(Logger *logger) const {
    std::lock_guard<std::mutex> lock(_lock);
    if (_objs.empty()) {
        return;
    }
    dumpHeader(logger);
    for (const auto &object: _objs) {
        if (object.second) {
            dumpObject(object.first,  object.second, logger);
        }
    }
    dumpFooter(logger);
}

Watcher* Watcher::instance() {
    static Watcher instance;
    return &instance;
}

NS_END