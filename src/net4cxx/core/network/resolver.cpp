//
// Created by yuwenyong on 17-11-22.
//

#include "net4cxx/core/network/resolver.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN


Resolver::Resolver(Reactor *reactor)
        : _resolver(reactor->getIOContext()){

}


void DelayedResolve::cancel() {
    auto resolver = _resolver.lock();
    if (!resolver) {
        NET4CXX_THROW_EXCEPTION(AlreadyCancelled, "");
    }
    resolver->cancel();
}


NS_END