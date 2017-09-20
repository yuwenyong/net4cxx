//
// Created by yuwenyong on 17-9-20.
//

#include "net4cxx/core/network/base.h"
#include "net4cxx/core/network/error.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN

Timeout::Timeout(Reactor *reactor)
        : _timer(reactor->getService()) {

}

void DelayedCall::cancel() {
    auto timeout = _timeout.lock();
    if (!timeout) {
        NET4CXX_THROW_EXCEPTION(AlreadyCancelled, "");
    }
    timeout->cancel();
}

NS_END