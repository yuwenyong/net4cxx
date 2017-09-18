//
// Created by yuwenyong on 17-9-18.
//

#ifndef NET4CXX_COMMON_DEBUGGING_WATCHER_H
#define NET4CXX_COMMON_DEBUGGING_WATCHER_H

#include "net4cxx/common/common.h"
#include <mutex>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/signals2.hpp>
#include "net4cxx/common/logging/logging.h"


NS_BEGIN

class NET4CXX_COMMON_API Watcher {
public:
    typedef std::map<std::string, int> ObjectMap;
    typedef std::function<bool (const std::string &, int)> Filter;
    typedef std::function<void (int, int)> SetCallback;
    typedef std::function<void (int, int, int)> IncCallback;
    typedef std::function<void (int, int, int)> DecCallback;
    typedef std::function<void (int)> DelCallback;
    typedef boost::signals2::signal<void (int, int)> SetCallbackSignal;
    typedef boost::signals2::signal<void (int, int, int)> IncCallbackSignal;
    typedef boost::signals2::signal<void (int, int, int)> DecCallbackSignal;
    typedef boost::signals2::signal<void (int)> DelCallbackSignal;
    typedef boost::ptr_map<std::string, SetCallbackSignal> SetCallbackContainer;
    typedef boost::ptr_map<std::string, IncCallbackSignal> IncCallbackContainer;
    typedef boost::ptr_map<std::string, DecCallbackSignal> DecCallbackContainer;
    typedef boost::ptr_map<std::string, DelCallbackSignal> DelCallbackContainer;

    Watcher() = default;
    Watcher(const Watcher&) = delete;
    Watcher& operator=(const Watcher&) = delete;
    ~Watcher() = default;

    void set(const char *key, int value);

    void inc(const char *key, int increment=1);

    void dec(const char *key, int decrement=1);

    void del(const char *key);

    void addSetCallback(const char *key, const SetCallback &callback);

    void addIncCallback(const char *key, const IncCallback &callback);

    void addDecCallback(const char *key, const DecCallback &callback);

    void addDelCallback(const char *key, const DelCallback &callback);

    void dump(Filter filter, Logger *logger=nullptr) const;

    void dumpAll(Logger *logger=nullptr) const;

    void dumpNonZero(Logger *logger=nullptr) const;

    static Watcher* instance();
protected:
    void dumpHeader(Logger *logger) const {
        if (logger == nullptr) {
            logger = Logging::getRootLogger();
        }
        NET4CXX_INFO(logger, "+----------------------------------------|--------------------+");
        NET4CXX_INFO(logger, "|%-40s|%-20s|", "ObjectKey", "CurrentValue");
    }

    void dumpObject(const std::string &key, int value, Logger *logger) const {
        if (logger == nullptr) {
            logger = Logging::getRootLogger();
        }
        NET4CXX_INFO(logger, "+----------------------------------------|--------------------+");
        NET4CXX_INFO(logger, "|%-40s|%-20d|", key.c_str(), value);
    }

    void dumpFooter(Logger *logger) const {
        if (logger == nullptr) {
            logger = Logging::getRootLogger();
        }
        NET4CXX_INFO(logger, "+----------------------------------------|--------------------+");
    }

    mutable std::mutex _lock;
    ObjectMap _objs;
    SetCallbackContainer _setCallbacks;
    IncCallbackContainer _incCallbacks;
    DecCallbackContainer _decCallbacks;
    DelCallbackContainer _delCallbacks;
};

NS_END

#define NET4CXX_Watcher  net4cxx::Watcher::instance()


#endif //NET4CXX_COMMON_DEBUGGING_WATCHER_H
