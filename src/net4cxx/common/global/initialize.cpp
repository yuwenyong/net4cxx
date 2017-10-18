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
    NET4CXX_Watcher->addIncCallback(NET4CXX_TCPServerConnection_COUNT, [](int oldValue, int increment, int value) {
        NET4CXX_TRACE(gGenLog, "Create TCPServerConnection, current count:%d", value);
    });
    NET4CXX_Watcher->addDecCallback(NET4CXX_TCPServerConnection_COUNT, [](int oldValue, int decrement, int value) {
        NET4CXX_TRACE(gGenLog, "Destroy TCPServerConnection, current count:%d", value);
    });

    NET4CXX_Watcher->addIncCallback(NET4CXX_TCPListener_COUNT, [](int oldValue, int increment, int value) {
        NET4CXX_TRACE(gGenLog, "Create TCPListener, current count:%d", value);
    });
    NET4CXX_Watcher->addDecCallback(NET4CXX_TCPListener_COUNT, [](int oldValue, int decrement, int value) {
        NET4CXX_TRACE(gGenLog, "Destroy TCPListener, current count:%d", value);
    });

    NET4CXX_Watcher->addIncCallback(NET4CXX_TCPClientConnection_COUNT, [](int oldValue, int increment, int value) {
        NET4CXX_TRACE(gGenLog, "Create TCPClientConnection, current count:%d", value);
    });
    NET4CXX_Watcher->addDecCallback(NET4CXX_TCPClientConnection_COUNT, [](int oldValue, int decrement, int value) {
        NET4CXX_TRACE(gGenLog, "Destroy TCPClientConnection, current count:%d", value);
    });

    NET4CXX_Watcher->addIncCallback(NET4CXX_TCPConnector_COUNT, [](int oldValue, int increment, int value) {
        NET4CXX_TRACE(gGenLog, "Create TCPConnector, current count:%d", value);
    });
    NET4CXX_Watcher->addDecCallback(NET4CXX_TCPConnector_COUNT, [](int oldValue, int decrement, int value) {
        NET4CXX_TRACE(gGenLog, "Destroy TCPConnector, current count:%d", value);
    });

    NET4CXX_Watcher->addIncCallback(NET4CXX_SSLServerConnection_COUNT, [](int oldValue, int increment, int value) {
        NET4CXX_TRACE(gGenLog, "Create SSLServerConnection, current count:%d", value);
    });
    NET4CXX_Watcher->addDecCallback(NET4CXX_SSLServerConnection_COUNT, [](int oldValue, int decrement, int value) {
        NET4CXX_TRACE(gGenLog, "Destroy SSLServerConnection, current count:%d", value);
    });

    NET4CXX_Watcher->addIncCallback(NET4CXX_SSLListener_COUNT, [](int oldValue, int increment, int value) {
        NET4CXX_TRACE(gGenLog, "Create SSLListener, current count:%d", value);
    });
    NET4CXX_Watcher->addDecCallback(NET4CXX_SSLListener_COUNT, [](int oldValue, int decrement, int value) {
        NET4CXX_TRACE(gGenLog, "Destroy SSLListener, current count:%d", value);
    });

    NET4CXX_Watcher->addIncCallback(NET4CXX_SSLClientConnection_COUNT, [](int oldValue, int increment, int value) {
        NET4CXX_TRACE(gGenLog, "Create SSLClientConnection, current count:%d", value);
    });
    NET4CXX_Watcher->addDecCallback(NET4CXX_SSLClientConnection_COUNT, [](int oldValue, int decrement, int value) {
        NET4CXX_TRACE(gGenLog, "Destroy SSLClientConnection, current count:%d", value);
    });

    NET4CXX_Watcher->addIncCallback(NET4CXX_SSLConnector_COUNT, [](int oldValue, int increment, int value) {
        NET4CXX_TRACE(gGenLog, "Create SSLConnector, current count:%d", value);
    });
    NET4CXX_Watcher->addDecCallback(NET4CXX_SSLConnector_COUNT, [](int oldValue, int decrement, int value) {
        NET4CXX_TRACE(gGenLog, "Destroy SSLConnector, current count:%d", value);
    });
#endif
}

NS_END