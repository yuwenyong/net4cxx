//
// Created by yuwenyong.vincent on 2018/10/5.
//

#include "net4cxx/shared/bootstrap/bootstrapper.h"
#include "net4cxx/shared/global/constants.h"
#include "net4cxx/common/configuration/options.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/debugging/crashreport.h"
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/common/logging/logging.h"
#include "net4cxx/common/utilities/objectmanager.h"
#include "net4cxx/core/network/reactor.h"
#include "net4cxx/shared/global/constants.h"
#include "net4cxx/shared/global/loggers.h"


NS_BEGIN

Bootstrapper* Bootstrapper::_instance = nullptr;


Bootstrapper::Bootstrapper() {
    if (_instance) {
        NET4CXX_THROW_EXCEPTION(AlreadyExist, "Bootstrapper object already exists");
    }
    _instance = this;
}

Bootstrapper::~Bootstrapper() {
    cleanup();
}

void Bootstrapper::run(const boost::function1<std::string, std::string> &name_mapper) {
    NET4CXX_ASSERT(!_inited);
    onInit();
    _inited = true;
    CrashReport::outputCrashReport();
    NET4CXX_Options->parseEnvironment(name_mapper);
    std::unique_ptr<Reactor> reactor;
    if (_reactorEnabled) {
        reactor = std::make_unique<Reactor>();
        _reactor = reactor.get();
        _reactor->makeCurrent();
    }
    onRun();
    if (reactor) {
        reactor->run();
    }
    onQuit();
    if (reactor) {
        _reactor->clearCurrent();
        _reactor = nullptr;
        reactor.reset();
    }
}

void Bootstrapper::run(int argc, const char *const *argv) {
    NET4CXX_ASSERT(!_inited);
    onInit();
    _inited = true;
    CrashReport::outputCrashReport();
    NET4CXX_Options->parseCommandLine(argc, argv);
    std::unique_ptr<Reactor> reactor;
    if (_reactorEnabled) {
        reactor = std::make_unique<Reactor>();
        _reactor = reactor.get();
        _reactor->makeCurrent();
    }
    onRun();
    if (reactor) {
        reactor->run();
    }
    onQuit();
    if (reactor) {
        _reactor->clearCurrent();
        _reactor = nullptr;
        reactor.reset();
    }
}

void Bootstrapper::run(const char *path) {
    NET4CXX_ASSERT(!_inited);
    onInit();
    _inited = true;
    CrashReport::outputCrashReport();
    NET4CXX_Options->parseConfigFile(path);
    std::unique_ptr<Reactor> reactor;
    if (_reactorEnabled) {
        reactor = std::make_unique<Reactor>();
        _reactor = reactor.get();
        _reactor->makeCurrent();
    }
    onRun();
    if (reactor) {
        reactor->run();
    }
    onQuit();
    if (reactor) {
        _reactor->clearCurrent();
        _reactor = nullptr;
        reactor.reset();
    }
}

void Bootstrapper::setCrashReportPath(const std::string &crashReportPath) {
    CrashReport::setCrashReportPath(crashReportPath);
}

void Bootstrapper::onInit() {

}

void Bootstrapper::onRun() {

}

void Bootstrapper::onQuit() {

}

void Bootstrapper::cleanup() {
    _reactor = nullptr;
    NET4CXX_ObjectManager->cleanup();
    NET4CXX_Watcher->dumpAll();
    Logging::close();
}


void BasicBootstrapper::onInit() {
    LogUtil::initGlobalLoggers();
    setupCommonWatchObjects();
}

void BasicBootstrapper::setupCommonWatchObjects() {
    NET4CXX_WATCH_OBJECT(WatchKeys::TCPServerConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::TCPListenerCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::TCPClientConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::TCPConnectorCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::SSLServerConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::SSLListenerCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::SSLClientConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::SSLConnectorCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXServerConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXListenerCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXClientConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXConnectorCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::UDPConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXDatagramConnectionCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::DeferredCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::IOStreamCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt8ReceiverCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt8ExtReceiverCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt16ReceiverCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt16ExtReceiverCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt32ReceiverCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::WebSocketServerProtocolCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::WebSocketClientProtocolCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::PeriodicCallbackCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::HTTPClientCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::HTTPClientConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::HTTPConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::HTTPServerRequestCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::RequestHandlerCount);
}


void AppBootstrapper::onInit() {
    BasicBootstrapper::onInit();
    LogUtil::defineLoggingOptions(NET4CXX_Options);
    enableReactor();
}


NS_END