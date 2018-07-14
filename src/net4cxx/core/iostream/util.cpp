//
// Created by yuwenyong.vincent on 2018/7/14.
//

#include "net4cxx/core/iostream/util.h"


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

NS_END