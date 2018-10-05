//
// Created by yuwenyong.vincent on 2018/7/8.
//

#ifndef NET4CXX_CORE_NETWORK_DEFER_H
#define NET4CXX_CORE_NETWORK_DEFER_H

#include "net4cxx/common/common.h"
#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/base.h"


NS_BEGIN

class Deferred;

class NET4CXX_COMMON_API DeferredValue {
public:
    struct tagDeferredEmpty {

    };

    struct tagDeferredError {
        tagDeferredError() = default;

        tagDeferredError(std::exception_ptr err): error(err) {}

        std::exception_ptr error;
    };


    struct tagDeferredValue {
        tagDeferredValue() = default;

        template <typename ValueT>
        tagDeferredValue(ValueT &&val): value(std::forward<ValueT>(val)) {}

        boost::any value;
    };

    struct tagDeferredSharedPtr {
        tagDeferredSharedPtr() = default;

        tagDeferredSharedPtr(std::shared_ptr<Deferred> val): value(std::move(val)) {}

        std::shared_ptr<Deferred> value;
    };

    struct tagDeferredWeakPtr {
        tagDeferredWeakPtr() = default;

        tagDeferredWeakPtr(const std::shared_ptr<Deferred> &val): value(val) {}

        std::weak_ptr<Deferred> value;
    };

    using ValueType = boost::variant<
            tagDeferredValue,
            tagDeferredEmpty,
            tagDeferredError,
            tagDeferredSharedPtr,
            tagDeferredWeakPtr>;

    DeferredValue() = default;

    DeferredValue(tagDeferredEmpty): _value(tagDeferredEmpty{}) {}

    template <typename ValueT>
    DeferredValue(ValueT &&value): _value(tagDeferredValue{std::forward<ValueT>(value)}) {}

    DeferredValue(nullptr_t): _value(tagDeferredValue{}) {}

    DeferredValue(std::exception_ptr error): _value(tagDeferredError(error)) {}

    DeferredValue(std::shared_ptr<Deferred> value): _value(tagDeferredSharedPtr{std::move(value)}) {}

    bool isNull() const {
        if (auto v = boost::get<const tagDeferredValue>(&_value)) {
            return v->value.empty();
        } else {
            return false;
        }
    }

    bool isEmpty() const {
        if (boost::get<const tagDeferredEmpty>(&_value)) {
            return true;
        } else {
            return false;
        }
    }

    bool isValue() const {
        if (auto v = boost::get<const tagDeferredValue>(&_value)) {
            return !v->value.empty();
        } else {
            return false;
        }
    }

    bool isError() const {
        if (boost::get<const tagDeferredError>(&_value)) {
            return true;
        } else {
            return false;
        }
    }

    bool isDeferred() const {
        struct IsDeferredVisitor: public boost::static_visitor<bool> {

            result_type operator()(const tagDeferredEmpty &v) const {
                return false;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return false;
            }

            result_type operator()(const tagDeferredError &v) const {
                return false;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return true;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return true;
            }
        };
        return boost::apply_visitor(IsDeferredVisitor(), _value);
    }

    template <typename ValueT>
    const ValueT* asValue() const {
        if (auto v = boost::get<const tagDeferredValue>(&_value)) {
            return boost::any_cast<const ValueT>(&v->value);;
        } else {
            return nullptr;
        }
    }

    template <typename ValueT>
    ValueT* asValue() {
        if (auto v = boost::get<tagDeferredValue>(&_value)) {
            return boost::any_cast<ValueT>(&v->value);;
        } else {
            return nullptr;
        }
    }

    std::exception_ptr asError() const {
        if (auto v = boost::get<const tagDeferredError>(&_value)) {
            return v->error;
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<Deferred> asDeferred() const {
        struct AsDeferredVisitor: public boost::static_visitor<std::shared_ptr<Deferred>> {
            using result_type = std::shared_ptr<Deferred>;

            result_type operator()(const tagDeferredEmpty &v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredError &v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return v.value;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return v.value.lock();
            }
        };
        return boost::apply_visitor(AsDeferredVisitor(), _value);
    }

    void throwError() const {
        if (auto v = boost::get<const tagDeferredError>(&_value)) {
            std::rethrow_exception(v->error);
        }
    }

    void releaseDeferred() {
        if (auto v = boost::get<const tagDeferredSharedPtr>(&_value)) {
            _value = tagDeferredWeakPtr{v->value};
        }
    }

    void restoreDeferred() {
        if (auto v = boost::get<const tagDeferredWeakPtr>(&_value)) {
            _value = tagDeferredSharedPtr{v->value.lock()};
        }
    }

    static const tagDeferredEmpty DeferredEmpty;
protected:
    ValueType _value;
};

class Reactor;

class NET4CXX_COMMON_API Deferred: public std::enable_shared_from_this<Deferred> {
public:
    using CallbackType = std::function<DeferredValue (DeferredValue)>;
    using CallbacksType = std::pair<CallbackType, CallbackType>;
    using CallbacksList = std::deque<CallbacksType>;
    using CancellerType = std::function<void (std::shared_ptr<Deferred>)>;
    using TimeoutCancelType = std::function<DeferredValue (DeferredValue)>;

    explicit Deferred(CancellerType canceller= nullptr)
            : _canceller(std::move(canceller)) {

    }

    virtual ~Deferred() = default;

    bool called() const {
        return _called;
    }

    const DeferredValue& result() const {
        return _result;
    }

    DeferredValue& result() {
        return _result;
    }

    void setCanceller(CancellerType canceller) {
        _canceller = std::move(canceller);
    }

    template <typename CallbackT>
    std::shared_ptr<Deferred> addCallbacks(CallbackT &&callback, CallbackType errback= nullptr) {
        if (errback) {
            _callbacks.emplace_back(std::forward<CallbackT>(callback), std::move(errback));
        } else {
            _callbacks.emplace_back(std::forward<CallbackT>(callback), [](DeferredValue value){ return value; });
        }
        if (_called) {
            runCallbacks();
        }
        return this->shared_from_this();
    }

    template <typename CallbackT>
    std::shared_ptr<Deferred> addCallback(CallbackT &&callback) {
        return addCallbacks(std::forward<CallbackT>(callback));
    }

    template <typename ErrbackT>
    std::shared_ptr<Deferred> addErrback(ErrbackT &&errback) {
        return addCallbacks([](DeferredValue value){ return value; }, std::forward<ErrbackT>(errback));
    }

    template <typename CallbackT>
    std::shared_ptr<Deferred> addBoth(CallbackT &&callback) {
        return addCallbacks(callback, callback);
    }

    std::shared_ptr<Deferred> addTimeout(double timeout, Reactor *reactor, TimeoutCancelType onTimeoutCancel= nullptr);

    std::shared_ptr<Deferred> chainDeferred(std::shared_ptr<Deferred> d) {
        d->_chainTo = this->shared_from_this();
        return addCallbacks([d](DeferredValue result) {
            d->callback(std::move(result));
            return nullptr;
        }, [d](DeferredValue result) {
            d->errback(std::move(result));
            return nullptr;
        });
    }

    void callback(DeferredValue &&result) {
        NET4CXX_ASSERT(!result.isEmpty());
        NET4CXX_ASSERT(!result.isError());
        NET4CXX_ASSERT(!result.isDeferred());
        startRunCallbacks(std::move(result));
    }

    void callback(const DeferredValue &result) {
        NET4CXX_ASSERT(!result.isEmpty());
        NET4CXX_ASSERT(!result.isError());
        NET4CXX_ASSERT(!result.isDeferred());
        startRunCallbacks(result);
    }

    template <typename ResultT>
    void callback(ResultT &&result) {
        static_assert(!std::is_same<typename RemoveCVR<ResultT>::type, DeferredValue::tagDeferredEmpty>::value, "");
        static_assert(!std::is_same<typename RemoveCVR<ResultT>::type, std::exception_ptr>::value, "");
        static_assert(!std::is_same<typename RemoveCVR<ResultT>::type, std::shared_ptr<Deferred>>::value, "");
        startRunCallbacks(std::forward<ResultT>(result));
    }

    void errback(const DeferredValue &result) {
        NET4CXX_ASSERT(result.isError() && result.asError());
        startRunCallbacks(result);
    }

    void errback(DeferredValue &&result) {
        NET4CXX_ASSERT(result.isError() && result.asError());
        startRunCallbacks(std::move(result));
    }

    void errback(std::exception_ptr error= nullptr) {
        if (!error) {
            error = std::current_exception();
        }
        NET4CXX_ASSERT(error);
        startRunCallbacks(error);
    }

    void pause() {
        ++_paused;
    }

    void unpause() {
        --_paused;
        NET4CXX_ASSERT(_paused >= 0);
        if (_paused > 0) {
            return;
        }
        if (_called) {
            runCallbacks();
        }
    }

    virtual void cancel();
protected:
    class Continuation {
    public:
        Continuation() = default;

        explicit Continuation(std::shared_ptr<Deferred> deferred)
                : _deferred(std::move(deferred)) {

        }

        std::shared_ptr<Deferred> getDeferred() const {
            return _deferred;
        }

        DeferredValue operator()(DeferredValue value) {
            return value;
        }
    protected:
        std::shared_ptr<Deferred> _deferred;
    };

    class RunningGuard {
    public:
        explicit RunningGuard(std::shared_ptr<Deferred> deferred)
                : _deferred(std::move(deferred)) {
            _deferred->_runningCallbacks = true;
        }

        ~RunningGuard() {
            _deferred->_runningCallbacks = false;
        }
    protected:
        std::shared_ptr<Deferred> _deferred;
    };

    template <typename ResultT>
    void startRunCallbacks(ResultT &&result) {
        if (_called) {
            if (_suppressAlreadyCalled) {
                _suppressAlreadyCalled = false;
                return;
            }
            NET4CXX_THROW_EXCEPTION(AlreadyCalledError, "");
        }
        _called = true;
        _result = std::forward<ResultT>(result);
        runCallbacks();
    }

    CallbacksType continuation() {
        return std::make_pair(Continuation(this->shared_from_this()), Continuation(this->shared_from_this()));
    }

    void runCallbacks();

    bool _called{false};
    bool _suppressAlreadyCalled{false};
    bool _runningCallbacks{false};
    int _paused{0};
    std::weak_ptr<Deferred> _chainTo;
    CallbacksList _callbacks;
    CancellerType _canceller;
    DeferredValue _result{DeferredValue::DeferredEmpty};
};

using DeferredPtr = std::shared_ptr<Deferred>;


class DeferredList: public Deferred {
public:
    explicit DeferredList(bool fireOnOneCallback = false, bool fireOnOneErrback = false, bool consumeErrors = false)
            : Deferred(nullptr)
            , _fireOnOneCallback(fireOnOneCallback)
            , _fireOnOneErrback(fireOnOneErrback)
            , _consumeErrors(consumeErrors) {

    }

    std::shared_ptr<Deferred> wait(std::vector<std::shared_ptr<Deferred>> deferredList);

    void cancel() override;

    const std::vector<DeferredValue>& results() const {
        return _resultList;
    }

    std::vector<DeferredValue>& results() {
        return _resultList;
    }
protected:
    DeferredValue cbDeferred(DeferredValue result, size_t index, bool succeeded);

    std::vector<std::weak_ptr<Deferred>> _deferredList;
    std::vector<DeferredValue> _resultList;
    bool _fireOnOneCallback{false};
    bool _fireOnOneErrback{false};
    bool _consumeErrors{false};
    size_t _finishedCount{0};
};

using DeferredListPtr = std::shared_ptr<DeferredList>;


inline DeferredPtr makeDeferred() {
    return std::make_shared<Deferred>();
}

template <typename ResultT>
DeferredPtr succeedDeferred(ResultT &&result) {
    auto d = makeDeferred();
    d->callback(std::forward<ResultT>(result));
    return d;
}

inline DeferredPtr failDeferred(std::exception_ptr error= nullptr) {
    auto d = makeDeferred();
    d->errback(error);
    return d;
}

template <typename CallableT, typename... Args>
DeferredPtr _executeDeferred(Type2Type<void> ignore, CallableT &&callable, Args&&... args) {
    try {
        callable(std::forward<Args>(args)...);
        return succeedDeferred(nullptr);
    } catch (...) {
        return failDeferred();
    }
}

template <typename ResultT, typename CallableT, typename... Args>
DeferredPtr _executeDeferred(Type2Type<ResultT> ignore, CallableT &&callable, Args&&... args) {
    try {
        ResultT result = callable(std::forward<Args>(args)...);
        return succeedDeferred(std::move(result));
    } catch (...) {
        return failDeferred();
    }
}

template <typename CallableT, typename... Args>
DeferredPtr executeDeferred(CallableT &&callable, Args&&... args) {
    using ResultT = typename std::result_of<CallableT>::type;
    return _executeDeferred(Type2Type<ResultT>(), std::forward<CallableT>(callable), std::forward<Args>(args)...);
}

template <typename CallableT, typename... Args>
DeferredPtr _maybeDeferred(Type2Type<DeferredPtr> ignore, CallableT &&callable, Args&&... args) {
    try {
        DeferredPtr result = callable(std::forward<Args>(args)...);
        return result;
    } catch (...) {
        return failDeferred();
    }
}

template <typename CallableT, typename... Args>
DeferredPtr _maybeDeferred(Type2Type<DeferredValue> ignore, CallableT &&callable, Args&&... args) {
    DeferredValue result{DeferredValue::DeferredEmpty};
    try {
        result = callable(std::forward<Args>(args)...);
    } catch (...) {
        return failDeferred();
    }
    if (result.isDeferred()) {
        return result.asDeferred();
    } else if (result.isError()) {
        return failDeferred(result.asError());
    }
    return succeedDeferred(std::move(result));
}

template <typename CallableT, typename... Args>
DeferredPtr _maybeDeferred(Type2Type<void> ignore, CallableT &&callable, Args&&... args) {
    try {
        callable(std::forward<Args>(args)...);
        return succeedDeferred(nullptr);
    } catch (...) {
        return failDeferred();
    }
}

template <typename ResultT, typename CallableT, typename... Args>
DeferredPtr _maybeDeferred(Type2Type<ResultT> ignore, CallableT &&callable, Args&&... args) {
    try {
        ResultT result = callable(std::forward<Args>(args)...);
        return succeedDeferred(std::move(result));
    } catch (...) {
        return failDeferred();
    }
}

template <typename CallableT, typename... Args>
DeferredPtr maybeDeferred(CallableT &&callable, Args&&... args) {
    using ResultT = typename std::result_of<CallableT>::type;
    return _maybeDeferred(Type2Type<ResultT>(), std::forward<CallableT>(callable), std::forward<Args>(args)...);
}

inline DeferredListPtr makeDeferredList(bool fireOnOneCallback=false, bool fireOnOneErrback=false,
                                        bool consumeErrors=false) {
    return std::make_shared<DeferredList>(fireOnOneCallback, fireOnOneErrback, consumeErrors);
}

inline DeferredListPtr gatherResults(std::vector<std::shared_ptr<Deferred>> deferredList, bool fireOnOneCallback=false,
                                     bool fireOnOneErrback=false, bool consumeErrors=false) {
    auto d = makeDeferredList(fireOnOneCallback, fireOnOneErrback, consumeErrors);
    d->wait(std::move(deferredList));
    return d;
}

NS_END

#endif //NET4CXX_CORE_NETWORK_DEFER_H
