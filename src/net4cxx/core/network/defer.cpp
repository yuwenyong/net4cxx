//
// Created by yuwenyong.vincent on 2018/7/8.
//

#include "net4cxx/core/network/defer.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN

const DeferredValue::tagDeferredEmpty DeferredValue::DeferredEmpty{};


std::shared_ptr<Deferred> Deferred::addTimeout(double timeout, Reactor *reactor, TimeoutCancelType onTimeoutCancel) {
    auto timeOut = std::make_shared<bool>(false);
    auto self = this->shared_from_this();

    auto delayedCall = reactor->callLater(timeout, [this, self, timeOut](){
        *timeOut = true;
        cancel();
    });

    addBoth([timeOut, onTimeoutCancel=std::move(onTimeoutCancel)](DeferredValue value){
        if (*timeOut) {
            if (onTimeoutCancel) {
                return onTimeoutCancel(value);
            } else {
                if (value.isError()) {
                    NET4CXX_THROW_EXCEPTION(TimeoutError, "Deferred");
                }
                return value;
            }
        }
        return value;
    });

    addBoth([delayedCall](DeferredValue result) mutable{
        if (delayedCall.active()) {
            delayedCall.cancel();
        }
        return result;
    });

    return self;
}

void Deferred::cancel() {
    if (!_called) {
        if (_canceller) {
            _canceller(this->shared_from_this());
        } else {
            _suppressAlreadyCalled = true;
        }
        if (!_called) {
            errback(std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(CancelledError, "")));
        }
    } else if (auto result = _result.asDeferred()){
        result->cancel();
    }
}

void Deferred::runCallbacks() {
    if (_runningCallbacks) {
        return;
    }

    std::vector<std::shared_ptr<Deferred>> chain = {this->shared_from_this()};
    std::shared_ptr<Deferred> current;
    bool finished;
    CallbackType callback;

    while (!chain.empty()) {
        current = chain.back();

        if (current->_paused > 0) {
            return;
        }

        finished = true;
        current->_chainTo.reset();

        while (!current->_callbacks.empty()) {
            if (current->_result.isError()) {
                callback = std::move(current->_callbacks.front().second);
            } else {
                callback = std::move(current->_callbacks.front().first);
            }
            current->_callbacks.pop_front();

            if (auto *cb = callback.template target<Continuation>()) {
                auto chainee = cb->getDeferred();
                chainee->_result = std::move(current->_result);
                current->_result = nullptr;

                --chainee->_paused;
                chain.emplace_back(std::move(chainee));
                finished = false;
                break;
            }

            try {
                RunningGuard guard(current);
                current->_result = callback(std::move(current->_result));
                NET4CXX_ASSERT(current->_result.asDeferred() != current);
            } catch (...) {
                current->_result = std::current_exception();
            }

            if (current->_result.isDeferred()) {
                auto resultDeferred = current->_result.asDeferred();
                NET4CXX_ASSERT(resultDeferred);
                auto &resultResult = resultDeferred->_result;

                if (resultResult.isEmpty() || resultResult.isDeferred() || resultDeferred->_paused > 0) {
                    current->pause();
                    current->_chainTo = resultDeferred;
                    current->_result.releaseDeferred();
                    resultDeferred->_callbacks.emplace_back(current->continuation());
                    break;
                } else {
                    resultDeferred->_result = nullptr;
                    current->_result = resultResult;
                }
            }
        }

        if (finished) {
            chain.pop_back();
        }
    }
}


std::shared_ptr<Deferred> DeferredList::wait(std::vector<std::shared_ptr<Deferred>> deferredList,
                                             bool fireOnOneCallback, bool fireOnOneErrback, bool consumeErrors) {
    NET4CXX_ASSERT(_deferredList.empty());
    for (auto &deferred: deferredList) {
        _deferredList.emplace_back(deferred);
    }
    _resultList.resize(_deferredList.size());
    if (_deferredList.empty() && !fireOnOneCallback) {
        callback(_resultList);
    }

    _fireOnOneCallback = fireOnOneCallback;
    _fireOnOneErrback = fireOnOneErrback;
    _consumeErrors = consumeErrors;
    _finishedCount = 0;

    auto self = shared_from_this();
    for (size_t index = 0; index != deferredList.size(); ++index) {
        deferredList[index]->addCallbacks([index, self, this](DeferredValue result) {
            return cbDeferred(std::move(result), index, true);
        }, [index, self, this](DeferredValue result) {
            return cbDeferred(std::move(result), index, false);
        });
    }
    return self;
}

void DeferredList::cancel() {
    if (!_called) {
        for (auto &deferred: _deferredList) {
            auto deferredPtr = deferred.lock();
            if (deferredPtr) {
                try {
                    deferredPtr->cancel();
                } catch (...) {
                    NET4CXX_ASSERT_MSG(false, "Exception raised from user supplied canceller");
                }
            }
        }
    }
}

DeferredValue DeferredList::cbDeferred(DeferredValue result, size_t index, bool succeeded) {
    _resultList[index] = std::move(result);

    ++_finishedCount;
    if (!_called) {
        if (succeeded && _fireOnOneCallback) {
            callback(_resultList[index]);
        } else if (!succeeded && _fireOnOneErrback) {
            errback(_resultList[index]);
        } else if (_finishedCount == _resultList.size()) {
            callback(_resultList);
        }
    }
    
    if (!succeeded && _consumeErrors) {
        _resultList[index] = nullptr;
    }

    return _resultList[index];
}

NS_END