//
// Created by yuwenyong.vincent on 2018/10/14.
//

#include "net4cxx/plugins/web/util.h"
#include "net4cxx/core/network/defer.h"


NS_BEGIN

void PeriodicCallback::run() {
    if (!_running) {
        return;
    }
    try {
        _callback();
    } catch (std::exception &e) {
        NET4CXX_LOG_ERROR(gAppLog, "Error in periodic callback:%s", e.what());
    }
    scheduleNext();
}


DeferredPtr sleepAsync(Reactor *reactor, const Duration &timeout) {
    auto d = makeDeferred();
    auto delayed = reactor->callLater(timeout, [d]() {
        d->callback(nullptr);
    });
    d->setCanceller([delayed](DeferredPtr) mutable {
        if (delayed.active()) {
            delayed.cancel();
        }
    });
    return d;
}

NS_END