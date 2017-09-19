//
// Created by yuwenyong on 17-9-19.
//

#include "net4cxx/common/global/initialize.h"
#include "net4cxx/common/configuration/options.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/common/logging/logging.h"
#include "net4cxx/common/utilities/objectmanager.h"

NS_BEGIN

bool GlobalInit::_inited = false;

void GlobalInit::initFromEnvironment(const boost::function1<std::string, std::string> &name_mapper) {
    BOOST_ASSERT(!_inited);
    LogUtil::initGlobalLoggers();
    LogUtil::defineLoggingOptions(NET4CXX_Options);
    NET4CXX_Options->parseEnvironment(name_mapper);
    if (!Logging::isInitialized()) {
        Logging::init();
    }
    setupWatcherHook();
    _inited = true;
}

void GlobalInit::initFromCommandLine(int argc, const char *const *argv) {
    BOOST_ASSERT(!_inited);
    LogUtil::initGlobalLoggers();
    LogUtil::defineLoggingOptions(NET4CXX_Options);
    NET4CXX_Options->parseCommandLine(argc, argv);
    if (!Logging::isInitialized()) {
        Logging::init();
    }
    setupWatcherHook();
    _inited = true;
}

void GlobalInit::initFromConfigFile(const char *path) {
    BOOST_ASSERT(!_inited);
    LogUtil::initGlobalLoggers();
    LogUtil::defineLoggingOptions(NET4CXX_Options);
    NET4CXX_Options->parseConfigFile(path);
    if (!Logging::isInitialized()) {
        Logging::init();
    }
    setupWatcherHook();
    _inited = true;
}

void GlobalInit::cleanup() {
    NET4CXX_ObjectManager->cleanup();
    NET4CXX_Watcher->dumpAll();
    Logging::close();
}

void GlobalInit::setupWatcherHook() {
#ifndef NET4CXX_NDEBUG

#endif
}

NS_END