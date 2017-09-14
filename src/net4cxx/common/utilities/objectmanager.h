//
// Created by yuwenyong on 17-9-14.
//

#ifndef NET4CXX_COMMON_UTILITIES_OBJECTMANAGER_H
#define NET4CXX_COMMON_UTILITIES_OBJECTMANAGER_H

#include "net4cxx/common/common.h"
#include <mutex>
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
    typedef std::list<CleanupObject *> CleanupObjectContainer;

    ObjectManager() = default;

    ~ObjectManager();

    template<typename ObjectT>
    Singleton<ObjectT> *registerObject() {
        std::lock_guard<std::mutex> lock(_objectsLock);
        if (cleaned()) {
            return nullptr;
        }
        Singleton<ObjectT> *object = boost::factory<Singleton<ObjectT> *>()();
        _cleanupObjects.push_back(object);
        return object;
    }

    void cleanup();

    bool cleaned() const {
        return _cleaned;
    }

    static ObjectManager *instance();

protected:
    bool _cleaned{false};
    CleanupObjectContainer _cleanupObjects;
    std::mutex _objectsLock;
};


template <typename ObjectT>
ObjectT * Singleton<ObjectT>::instance() {
    if (!_singleton) {
        _singleton = ObjectManager::instance()->registerObject<ObjectT>();
    }
    return _singleton ? &(_singleton->_instance) : nullptr;
}

NS_END

#define NET4CXX_OBJECT_MGR  net4cxx::ObjectManager::instance()

#endif //NET4CXX_COMMON_UTILITIES_OBJECTMANAGER_H
