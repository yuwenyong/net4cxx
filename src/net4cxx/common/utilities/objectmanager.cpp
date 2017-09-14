//
// Created by yuwenyong on 17-9-14.
//

#include "net4cxx/common/utilities/objectmanager.h"
#include <boost/foreach.hpp>


NS_BEGIN

ObjectManager::~ObjectManager() {
    cleanup();
}

void ObjectManager::cleanup() {
    std::lock_guard<std::mutex> lock(_objectsLock);
    if (cleaned()) {
        return;
    }
    BOOST_REVERSE_FOREACH(CleanupObject *object, _cleanupObjects) { object->cleanup(); }
    _cleanupObjects.clear();
    _cleaned = true;
}

ObjectManager* ObjectManager::instance() {
    static ObjectManager instance;
    return &instance;
}

NS_END