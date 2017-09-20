//
// Created by yuwenyong on 17-9-20.
//

#include "net4cxx/core/network/reactor.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/common/utilities/random.h"
#include "net4cxx/core/network/error.h"


NS_BEGIN

thread_local Reactor* Reactor::_current = nullptr;

Reactor::Reactor()
        : _ioService()
        , _signalSet(_ioService) {

}

void Reactor::run(bool installSignalHandlers) {
    if (_running) {
        NET4CXX_THROW_EXCEPTION(ReactorAlreadyRunning, "");
    }
    Random::seed();
    if (_ioService.stopped()) {
        _ioService.reset();
    }
    Reactor *oldCurrent = _current;
    _current = this;
    WorkType work(_ioService);
    startRunning(installSignalHandlers);
    _running = false;
    _current = oldCurrent;
}

void Reactor::stop() {
    if (_ioService.stopped()) {
        NET4CXX_THROW_EXCEPTION(ReactorNotRunning, "Can't stop reactor that isn't running.");
    }
    if (!_stopCallbacks.empty()) {
        _stopCallbacks();
        _stopCallbacks.disconnect_all_slots();
    }
    _ioService.stop();
}

void Reactor::startRunning(bool installSignalHandlers) {
    if (installSignalHandlers) {
        _installSignalHandlers = installSignalHandlers;
        handleSignals();
    }
    _running = true;
    while (!_ioService.stopped()) {
        try {
            _ioService.run();
        } catch (std::exception &e) {
            NET4CXX_ERROR(gAppLog, "Unexpected Exception:%s", e.what());
        } catch (...) {
            NET4CXX_ERROR(gAppLog, "Unknown Exception:%s");
        }
    }
}

void Reactor::handleSignals() {
    _signalSet.cancel();
    _signalSet.clear();
    _signalSet.add(SIGINT);
    _signalSet.add(SIGTERM);
#if defined(SIGBREAK)
    _signalSet.add(SIGBREAK);
#endif
#if defined(SIGQUIT)
    _signalSet.add(SIGQUIT);
#endif
    _signalSet.async_wait(std::bind(&Reactor::onSignal, this,  std::placeholders::_1, std::placeholders::_2));
}

void Reactor::onSignal(const boost::system::error_code &ec, int signalNumber) {
    if (!ec) {
        if (signalNumber == SIGINT) {
            sigInt();
        }
        if (signalNumber == SIGTERM) {
            sigTerm();
        }
#if defined(SIGBREAK)
        if (signalNumber == SIGBREAK) {
            sigBreak();
        }
#endif
#if defined(SIGQUIT)
        if (signalNumber == SIGQUIT) {
            sigQuit();
        }
#endif
    }
}

void Reactor::sigInt() {
    NET4CXX_INFO(gGenLog, "Received SIGINT, shutting down.");
    stop();
}

void Reactor::sigTerm() {
    NET4CXX_INFO(gGenLog, "Received SIGINT, shutting down.");
    stop();
}

void Reactor::sigBreak() {
    NET4CXX_INFO(gGenLog, "Received SIGBREAK, shutting down.");
    stop();
}

void Reactor::sigQuit() {
    NET4CXX_INFO(gGenLog, "Received SIGQUIT, shutting down.");
    stop();
}


NS_END