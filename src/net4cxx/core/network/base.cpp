//
// Created by yuwenyong on 17-9-20.
//

#include "net4cxx/core/network/base.h"
#include "net4cxx/common/debugging/assert.h"
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


void Connection::dataReceived(Byte *data, size_t length) {
    BOOST_ASSERT(_protocol);
    _protocol->dataReceived(data, length);
}

void Connection::connectionLost(std::exception_ptr reason) {
    BOOST_ASSERT(_protocol);
    _protocol->connectionLost(std::move(reason));
}


NS_END