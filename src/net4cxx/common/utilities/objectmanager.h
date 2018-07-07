//
// Created by yuwenyong on 17-9-14.
//

#ifndef NET4CXX_COMMON_UTILITIES_OBJECTMANAGER_H
#define NET4CXX_COMMON_UTILITIES_OBJECTMANAGER_H

#include "net4cxx/common/common.h"
#include <mutex>
#include <typeindex>
#include <boost/checked_delete.hpp>
#include <boost/functional/factory.hpp>

NS_BEGIN

class CleanupObject {
public:
    virtual ~CleanupObject() = default;

    virtual void cleanup() = 0;
};

template<typename ObjectT>
class Singleton : public CleanupObject {
public:
    friend class ObjectManager;
    friend class boost::factory<Singleton<ObjectT> *>;

    static ObjectT *instance();
protected:
    void cleanup() override {
        boost::checked_delete(this);
        _singleton = nullptr;
    }

    Singleton() = default;

    ObjectT _instance;
    static Singleton<ObjectT> *_singleton;
};

template<typename ObjectT>
Singleton<ObjectT> *Singleton<ObjectT>::_singleton = nullptr;


class ObjectManager {
public:
    typedef std::list<CleanupObject *> CleanupObjectList;
    typedef std::map<std::type_index, CleanupObject *> CleanupObjectMap;

    ObjectManager() = default;

    ~ObjectManager();

    template<typename ObjectT>
    Singleton<ObjectT> *registerObject() {
        std::lock_guard<std::mutex> lock(_lock);
        if (_cleaned) {
            return nullptr;
        }
        auto iter = _cleanupObjectMap.find(std::type_index(typeid(ObjectT)));
        if (iter != _cleanupObjectMap.end()) {
            return static_cast<Singleton<ObjectT> *>(iter->second);
        }
        Singleton<ObjectT> *object = boost::factory<Singleton<ObjectT> *>()();
        _cleanupObjectMap[std::type_index(typeid(ObjectT))] = object;
        _cleanupObjectList.push_back(object);
        return object;
    }

    void cleanup();

    bool cleaned() const {
        std::lock_guard<std::mutex> lock(_lock);
        return _cleaned;
    }

    static ObjectManager *instance();

protected:
    mutable std::mutex _lock;
    bool _cleaned{false};
    CleanupObjectList _cleanupObjectList;
    CleanupObjectMap _cleanupObjectMap;
};


template <typename ObjectT>
ObjectT * Singleton<ObjectT>::instance() {
    if (!_singleton) {
        _singleton = ObjectManager::instance()->registerObject<ObjectT>();
    }
    return _singleton ? &(_singleton->_instance) : nullptr;
}

NS_END

#define NET4CXX_ObjectManager  net4cxx::ObjectManager::instance()

#endif //NET4CXX_COMMON_UTILITIES_OBJECTMANAGER_H
