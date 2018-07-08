//
// Created by yuwenyong.vincent on 2018/7/8.
//

#ifndef NET4CXX_CORE_NETWORK_DEFER_H
#define NET4CXX_CORE_NETWORK_DEFER_H

#include "net4cxx/common/common.h"
#include <boost/optional.hpp>
#include "net4cxx/core/network/base.h"


NS_BEGIN


template <typename... Args>
class Deferred: public std::enable_shared_from_this<Deferred<Args...>> {
public:
    template <typename CallbackT>
    std::shared_ptr<Deferred<Args...>> addCallback(CallbackT &&callback) {
        _callback = std::forward<CallbackT>(callback);
        if (_called) {
            runCallback();
        }
        return this->shared_from_this();
    }

    template <typename ErrbackT>
    std::shared_ptr<Deferred<Args...>> addErrback(ErrbackT &&errback) {
        _errback = std::forward<ErrbackT>(errback);
        if (_called) {
            runCallback();
        }
        return this->shared_from_this();
    }

    template <typename CallbackT, typename ErrbackT>
    std::shared_ptr<Deferred<Args...>> addBoth(CallbackT &&callback, ErrbackT &&errback) {
        _callback = std::forward<CallbackT>(callback);
        _errback = std::forward<ErrbackT>(errback);
        if (_called) {
            runCallback();
        }
        return this->shared_from_this();
    }

    void callback(Args... args) {
        startRunCallback(std::forward<Args>(args)...);
    }

    void errback(std::exception_ptr error= nullptr) {
        if (!error) {
            error = std::current_exception();
        }
        startRunErrback(error);
    }

    void errback(std::exception &e) {
        startRunErrback(std::make_exception_ptr(e));
    }
protected:
    void runCallback();

    template<std::size_t... I>
    void runSuccessCallback(std::index_sequence<I...>) {
        _callback(std::get<I>(*_result)...);
    }

    void startRunCallback(Args... args) {
        if (_called) {
            NET4CXX_THROW_EXCEPTION(AlreadyCalledError, "");
        }
        _called = true;
        _result = std::make_tuple(std::forward<Args>(args)...);
        _succeed = true;
        runCallback();
    }

    void startRunErrback(std::exception_ptr error) {
        if (_called) {
            NET4CXX_THROW_EXCEPTION(AlreadyCalledError, "");
        }
        _called = true;
        _error = error;
        _succeed = false;
        runCallback();
    }

    bool _called{false};
    bool _succeed{false};
    std::function<void (Args...)> _callback;
    std::function<void (std::exception_ptr)> _errback;
    boost::optional<std::tuple<Args...>> _result;
    std::exception_ptr _error;
};


template <typename... Args>
void Deferred<Args...>::runCallback() {
    if (_succeed) {
        if (_callback) {
            runSuccessCallback(std::make_index_sequence<sizeof...(Args)>());
            _callback = nullptr;
        }
    } else {
        if (_errback) {
            _errback(_error);
            _errback = nullptr;
        }
    }
}


template <>
class Deferred<>: public std::enable_shared_from_this<Deferred<>> {
public:
    template <typename CallbackT>
    std::shared_ptr<Deferred<>> addCallback(CallbackT &&callback) {
        _callback = std::forward<CallbackT>(callback);
        if (_called) {
            runCallback();
        }
        return this->shared_from_this();
    }

    template <typename ErrbackT>
    std::shared_ptr<Deferred<>> addErrback(ErrbackT &&errback) {
        _errback = std::forward<ErrbackT>(errback);
        if (_called) {
            runCallback();
        }
        return this->shared_from_this();
    }

    template <typename CallbackT, typename ErrbackT>
    std::shared_ptr<Deferred<>> addBoth(CallbackT &&callback, ErrbackT &&errback) {
        _callback = std::forward<CallbackT>(callback);
        _errback = std::forward<ErrbackT>(errback);
        if (_called) {
            runCallback();
        }
        return this->shared_from_this();
    }

    void callback() {
        startRunCallback();
    }

    void errback(std::exception_ptr error= nullptr) {
        if (!error) {
            error = std::current_exception();
        }
        startRunErrback(error);
    }

    void errback(std::exception &e) {
        startRunErrback(std::make_exception_ptr(e));
    }
protected:
    void runCallback();

    void startRunCallback() {
        if (_called) {
            NET4CXX_THROW_EXCEPTION(AlreadyCalledError, "");
        }
        _called = true;
        _succeed = true;
        runCallback();
    }

    void startRunErrback(std::exception_ptr error) {
        if (_called) {
            NET4CXX_THROW_EXCEPTION(AlreadyCalledError, "");
        }
        _called = true;
        _error = error;
        _succeed = false;
        runCallback();
    }

    bool _called{false};
    bool _succeed{false};
    std::function<void ()> _callback;
    std::function<void (std::exception_ptr)> _errback;
    std::exception_ptr _error;
};

void Deferred<>::runCallback() {
    if (_succeed) {
        if (_callback) {
            _callback();
            _callback = nullptr;
        }

    } else {
        if (_errback) {
            _errback(_error);
            _errback = nullptr;
        }
    }
}


template <typename... Args>
using DeferredPtr = std::shared_ptr<Deferred<Args...>>;


template <typename... Args>
DeferredPtr<Args...> makeDeferred() {
    return std::make_shared<Deferred<Args...>>();
}


NS_END

#endif //NET4CXX_CORE_NETWORK_DEFER_H
