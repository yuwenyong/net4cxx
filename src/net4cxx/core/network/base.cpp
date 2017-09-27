//
// Created by yuwenyong on 17-9-20.
//

#include "net4cxx/core/network/base.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/error.h"
#include "net4cxx/core/network/protocol.h"
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


const char* Port::logPrefix() const {
    return "Port";
}


const char* Connection::logPrefix() const {
    return "Connection";
}

void Connection::dataReceived(Byte *data, size_t length) {
    auto protocol = _protocol.lock();
    BOOST_ASSERT(protocol);
    protocol->dataReceived(data, length);
}

void Connection::connectionLost(std::exception_ptr reason) {
    auto protocol = _protocol.lock();
    BOOST_ASSERT(protocol);
    protocol->connectionLost(std::move(reason));
}

NS_END