//
// Created by yuwenyong.vincent on 2018/7/8.
//

#ifndef NET4CXX_CORE_NETWORK_DEFER_H
#define NET4CXX_CORE_NETWORK_DEFER_H

#include "net4cxx/common/common.h"
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/base.h"


NS_BEGIN


struct tagDeferredEmpty {

};

extern tagDeferredEmpty DeferredEmpty;

struct tagDeferredNull {

};

extern tagDeferredNull DeferredNull;


struct tagDeferredError {
    tagDeferredError() = default;

    tagDeferredError(std::exception_ptr err): error(err) {}

    std::exception_ptr error;
};


template <typename ValueT>
class Deferred;


template <typename ValueT>
class DeferredValue {
public:
    struct tagDeferredValue {
        tagDeferredValue() = default;

        tagDeferredValue(ValueT &&val): value(std::forward<ValueT>(val)) {}

        template <typename ArgT>
        tagDeferredValue(ArgT &&val): value(std::forward<ValueT>(val)) {}

        ValueT value;
    };

    struct tagDeferredSharedPtr {
        tagDeferredSharedPtr() = default;

        tagDeferredSharedPtr(std::shared_ptr<Deferred<ValueT>> val): value(std::move(val)) {}

        std::shared_ptr<Deferred<ValueT>> value;
    };

    struct tagDeferredWeakPtr {
        tagDeferredWeakPtr() = default;

        tagDeferredWeakPtr(const std::shared_ptr<Deferred<ValueT>> &val): value(val) {}

        std::weak_ptr<Deferred<ValueT>> value;
    };

    using ValueType = boost::variant<
            tagDeferredNull,
            tagDeferredEmpty,
            tagDeferredValue,
            tagDeferredError,
            tagDeferredSharedPtr,
            tagDeferredWeakPtr>;

    DeferredValue() = default;

    DeferredValue(tagDeferredNull): _value(tagDeferredNull{}) {}

    DeferredValue(tagDeferredEmpty): _value(tagDeferredEmpty{}) {}

    DeferredValue(ValueT &&value): _value(tagDeferredValue{std::forward<ValueT>(value)}) {}

    DeferredValue(nullptr_t): _value(tagDeferredValue{nullptr}) {}

    template <typename ArgT>
    DeferredValue(ArgT &&arg): _value(tagDeferredValue{std::forward<ValueT>(arg)}) {}

    DeferredValue(std::exception_ptr error): _value(tagDeferredError(error)) {}

    DeferredValue(std::shared_ptr<Deferred<ValueT>> value): _value(tagDeferredSharedPtr{std::move(value)}) {}

    bool isNull() const {
        struct IsNullVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return true;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return false;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return false;
            }

            result_type operator()(const tagDeferredError &v) const {
                return false;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return false;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return false;
            }
        };
        return boost::apply_visitor(IsNullVisitor(), _value);
    }

    bool isEmpty() const {
        struct IsEmptyVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return false;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return true;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return false;
            }

            result_type operator()(const tagDeferredError &v) const {
                return false;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return false;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return false;
            }
        };
        return boost::apply_visitor(IsEmptyVisitor(), _value);
    }

    bool isValue() const {
        struct IsValueVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return false;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return false;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return true;
            }

            result_type operator()(const tagDeferredError &v) const {
                return false;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return false;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return false;
            }
        };
        return boost::apply_visitor(IsValueVisitor(), _value);
    }

    bool isError() const {
        struct IsValueVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return false;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return false;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return false;
            }

            result_type operator()(const tagDeferredError &v) const {
                return true;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return false;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return false;
            }
        };
        return boost::apply_visitor(IsValueVisitor(), _value);
    }

    bool isDeferred() const {
        struct IsDeferredVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return false;
            }

            result_type operator()(tagDeferredEmpty v) const {
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

    bool isAssigned() const {
        struct IsAssignedVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return true;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return false;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return true;
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
        return boost::apply_visitor(IsAssignedVisitor(), _value);
    }

    const ValueT* asValue() const {
        struct AsValueVisitor: public boost::static_visitor<const ValueT*> {
            using result_type = const ValueT*;

            result_type operator()(tagDeferredNull v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return &v.value;
            }

            result_type operator()(const tagDeferredError &v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return nullptr;
            }
        };
        return boost::apply_visitor(AsValueVisitor(), _value);
    }

    ValueT* asValue() {
        struct AsValueVisitor: public boost::static_visitor<ValueT *> {
            using result_type = ValueT*;

            result_type operator()(tagDeferredNull v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredValue &v) const {
                return &v.value;
            }

            result_type operator()(tagDeferredError &v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredSharedPtr &v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredWeakPtr &v) const {
                return nullptr;
            }
        };
        return boost::apply_visitor(AsValueVisitor(), _value);
    }

    std::exception_ptr asError() const {
        struct AsErrorVisitor: public boost::static_visitor<std::exception_ptr> {
            result_type operator()(tagDeferredNull v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredValue &v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredError &v) const {
                return v.error;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return nullptr;
            }
        };
        return boost::apply_visitor(AsErrorVisitor(), _value);
    }

    std::shared_ptr<Deferred<ValueT>> asDeferred() const {
        struct AsDeferredVisitor: public boost::static_visitor<std::shared_ptr<Deferred<ValueT>>> {
            using result_type = std::shared_ptr<Deferred<ValueT>>;

            result_type operator()(tagDeferredNull v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredEmpty v) const {
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
        struct ThrowErrorVisitor: public boost::static_visitor<void> {
            result_type operator()(tagDeferredNull v) const {

            }

            result_type operator()(tagDeferredEmpty v) const {

            }

            result_type operator()(const tagDeferredValue &v) const {

            }

            result_type operator()(const tagDeferredError &v) const {
                if (v.error) {
                    std::rethrow_exception(v.error);
                }
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {

            }

            result_type operator()(const tagDeferredWeakPtr &v) const {

            }
        };
        boost::apply_visitor(ThrowErrorVisitor(), _value);
    }

    void releaseDeferred() {
        struct ReleaseDeferredVisitor: public boost::static_visitor<void> {
            explicit ReleaseDeferredVisitor(ValueType *val): value(val) {}

            result_type operator()(tagDeferredNull v) const {

            }

            result_type operator()(tagDeferredEmpty v) const {

            }

            result_type operator()(tagDeferredValue &v) const {

            }

            result_type operator()(tagDeferredError &v) const {

            }

            result_type operator()(tagDeferredSharedPtr &v) const {
                *value = tagDeferredWeakPtr{v.value};
            }

            result_type operator()(tagDeferredWeakPtr &v) const {

            }

            ValueType *value;
        };
        boost::apply_visitor(ReleaseDeferredVisitor(&_value), _value);
    }

    void restoreDeferred() {
        struct RestoreDeferredVisitor: public boost::static_visitor<void> {
            explicit RestoreDeferredVisitor(ValueType *val): value(val) {}

            result_type operator()(tagDeferredNull v) const {

            }

            result_type operator()(tagDeferredEmpty v) const {

            }

            result_type operator()(tagDeferredValue &v) const {

            }

            result_type operator()(tagDeferredError &v) const {

            }

            result_type operator()(tagDeferredSharedPtr &v) const {

            }

            result_type operator()(tagDeferredWeakPtr &v) const {
                *value = tagDeferredSharedPtr{v.value.lock()};
            }

            ValueType *value;
        };
        boost::apply_visitor(RestoreDeferredVisitor(&_value), _value);
    }
protected:
    ValueType _value;
};


template <>
class DeferredValue<void> {
public:
    struct tagDeferredSharedPtr {
        tagDeferredSharedPtr() = default;

        tagDeferredSharedPtr(std::shared_ptr<Deferred<void>> val): value(std::move(val)) {}

        std::shared_ptr<Deferred<void>> value;
    };

    struct tagDeferredWeakPtr {
        tagDeferredWeakPtr() = default;

        tagDeferredWeakPtr(const std::shared_ptr<Deferred<void>> &val): value(val) {}

        std::weak_ptr<Deferred<void>> value;
    };

    using ValueType = boost::variant<
            tagDeferredNull,
            tagDeferredEmpty,
            tagDeferredError,
            tagDeferredSharedPtr,
            tagDeferredWeakPtr>;

    DeferredValue() = default;

    DeferredValue(tagDeferredNull): _value(tagDeferredNull{}) {}

    DeferredValue(tagDeferredEmpty): _value(tagDeferredEmpty{}) {}

    DeferredValue(std::exception_ptr error): _value(tagDeferredError(error)) {}

    DeferredValue(std::shared_ptr<Deferred<void>> value): _value(tagDeferredSharedPtr{std::move(value)}) {}

    bool isNull() const {
        struct IsNullVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return true;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return false;
            }

            result_type operator()(const tagDeferredError &v) const {
                return false;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return false;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return false;
            }
        };
        return boost::apply_visitor(IsNullVisitor(), _value);
    }

    bool isEmpty() const {
        struct IsEmptyVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return false;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return true;
            }

            result_type operator()(const tagDeferredError &v) const {
                return false;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return false;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return false;
            }
        };
        return boost::apply_visitor(IsEmptyVisitor(), _value);
    }

    bool isError() const {
        struct IsValueVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return false;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return false;
            }

            result_type operator()(const tagDeferredError &v) const {
                return true;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return false;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return false;
            }
        };
        return boost::apply_visitor(IsValueVisitor(), _value);
    }

    bool isDeferred() const {
        struct IsDeferredVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return false;
            }

            result_type operator()(tagDeferredEmpty v) const {
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

    bool isAssigned() const {
        struct IsAssignedVisitor: public boost::static_visitor<bool> {
            result_type operator()(tagDeferredNull v) const {
                return true;
            }

            result_type operator()(tagDeferredEmpty v) const {
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
        return boost::apply_visitor(IsAssignedVisitor(), _value);
    }

    std::exception_ptr asError() const {
        struct AsErrorVisitor: public boost::static_visitor<std::exception_ptr> {
            result_type operator()(tagDeferredNull v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredEmpty v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredError &v) const {
                return v.error;
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {
                return nullptr;
            }

            result_type operator()(const tagDeferredWeakPtr &v) const {
                return nullptr;
            }
        };
        return boost::apply_visitor(AsErrorVisitor(), _value);
    }

    std::shared_ptr<Deferred<void>> asDeferred() const {
        struct AsDeferredVisitor: public boost::static_visitor<std::shared_ptr<Deferred<void>>> {
            using result_type = std::shared_ptr<Deferred<void>>;

            result_type operator()(tagDeferredNull v) const {
                return nullptr;
            }

            result_type operator()(tagDeferredEmpty v) const {
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
        struct ThrowErrorVisitor: public boost::static_visitor<void> {
            result_type operator()(tagDeferredNull v) const {

            }

            result_type operator()(tagDeferredEmpty v) const {

            }

            result_type operator()(const tagDeferredError &v) const {
                if (v.error) {
                    std::rethrow_exception(v.error);
                }
            }

            result_type operator()(const tagDeferredSharedPtr &v) const {

            }

            result_type operator()(const tagDeferredWeakPtr &v) const {

            }
        };
        boost::apply_visitor(ThrowErrorVisitor(), _value);
    }

    void releaseDeferred() {
        struct ReleaseDeferredVisitor: public boost::static_visitor<void> {
            explicit ReleaseDeferredVisitor(ValueType *val): value(val) {}

            result_type operator()(tagDeferredNull v) const {

            }

            result_type operator()(tagDeferredEmpty v) const {

            }

            result_type operator()(tagDeferredError &v) const {

            }

            result_type operator()(tagDeferredSharedPtr &v) const {
                *value = tagDeferredWeakPtr{v.value};
            }

            result_type operator()(tagDeferredWeakPtr &v) const {

            }

            ValueType *value;
        };
        boost::apply_visitor(ReleaseDeferredVisitor(&_value), _value);
    }

    void restoreDeferred() {
        struct RestoreDeferredVisitor: public boost::static_visitor<void> {
            explicit RestoreDeferredVisitor(ValueType *val): value(val) {}

            result_type operator()(tagDeferredNull v) const {

            }

            result_type operator()(tagDeferredEmpty v) const {

            }

            result_type operator()(tagDeferredError &v) const {

            }

            result_type operator()(tagDeferredSharedPtr &v) const {

            }

            result_type operator()(tagDeferredWeakPtr &v) const {
                *value = tagDeferredSharedPtr{v.value.lock()};
            }

            ValueType *value;
        };
        boost::apply_visitor(RestoreDeferredVisitor(&_value), _value);
    }
protected:
    ValueType _value;
};


template <typename ValueT>
class Deferred: public std::enable_shared_from_this<Deferred<ValueT>> {
public:
    using CallbackType = std::function<void (DeferredValue<ValueT> &)>;
    using CallbacksType = std::pair<CallbackType, CallbackType>;
    using CallbacksList = std::deque<CallbacksType>;
    using CancellerType = std::function<void (std::shared_ptr<Deferred<ValueT>>)>;

    explicit Deferred(CancellerType canceller= nullptr)
            : _canceller(std::move(canceller)) {

    }

    bool called() const {
        return _called;
    }

    DeferredValue<ValueT>& result() {
        return _result;
    }

    template <typename CallbackT, typename ErrbackT>
    std::shared_ptr<Deferred<ValueT>> addCallbacks(CallbackT &&callback, ErrbackT &&errback= nullptr) {
        NET4CXX_ASSERT(callback);
        if (errback) {
            _callbacks.emplace_back(std::forward<CallbackT>(callback), std::forward<ErrbackT>(errback));
        } else {
            _callbacks.emplace_back(std::forward<CallbackT>(callback), [](DeferredValue<ValueT> &value){});
        }
        if (_called) {
            runCallbacks();
        }
        return this->shared_from_this();
    }

    template <typename CallbackT>
    std::shared_ptr<Deferred<ValueT>> addCallback(CallbackT &&callback) {
        return addCallbacks(std::forward<CallbackT>(callback), [](DeferredValue<ValueT> &value){});
    }

    template <typename ErrbackT>
    std::shared_ptr<Deferred<ValueT>> addErrback(ErrbackT &&errback) {
        return addCallbacks([](DeferredValue<ValueT> &value){}, std::forward<ErrbackT>(errback));
    }

    template <typename CallbackT>
    std::shared_ptr<Deferred<ValueT>> addBoth(CallbackT &&callback) {
        return addCallbacks(callback, callback);
    }

    template <typename ClockT, typename TimeoutCancelT>
    std::shared_ptr<Deferred<ValueT>> addTimeout(double timeout, ClockT *clock,
                                                 TimeoutCancelT &&onTimeoutCancel= nullptr) {
        auto timeOut = std::make_shared<bool>(false);
        auto self = this->shared_from_this();

        auto delayedCall = clock->callLater(timeout, [this, self, timeOut](){
            *timeOut = true;
            cancel();
        });

        addBoth([timeOut, onTimeoutCancel=std::forward<TimeoutCancelT>(onTimeoutCancel)](DeferredValue<ValueT> &value){
            if (*timeOut) {
                if (onTimeoutCancel) {
                    onTimeoutCancel(value);
                } else {
                    if (value.isError()) {
                        NET4CXX_THROW_EXCEPTION(TimeoutError, "Deferred");
                    }
                }
            }
        });

        addBoth([delayedCall](DeferredValue<ValueT> &result){
            if (delayedCall.active()) {
                delayedCall.cancel();
            }
        });

        return self;
    }

    std::shared_ptr<Deferred<ValueT>> chainDeferred(std::shared_ptr<Deferred<ValueT>> d) {
        d->_chainTo = this->shared_from_this();
        return addCallbacks([d](DeferredValue<ValueT> &result) {
            d->callback(result);
            result = DeferredNull;
        }, [d](DeferredValue<ValueT> &result) {
            d->errback(result);
            result = DeferredNull;
        });
    }

    void callback(const DeferredValue<ValueT> &result) {
        NET4CXX_ASSERT(!result.isEmpty());
        NET4CXX_ASSERT(!result.isError());
        NET4CXX_ASSERT(!result.isDeferred());
        startRunCallbacks(result);
    }

    template <typename ResultT>
    void callback(ResultT &&result) {
        static_assert(!std::is_same<ResultT, tagDeferredEmpty>::value, "");
        static_assert(!std::is_same<ResultT, std::exception_ptr>::value, "");
        static_assert(!std::is_same<ResultT, std::shared_ptr<Deferred<ValueT>>>::value, "");
        startRunCallbacks(std::forward<ResultT>(result));
    }

    void errback(const DeferredValue<ValueT> &result) {
        NET4CXX_ASSERT(result.isError() && result.asError());
        startRunCallbacks(result);
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

    void cancel() {
        if (!_called) {
            if (_canceller) {
                _canceller(this->shared_from_this());
            } else {
                _suppressAlreadyCalled = true;
            }
            if (!_called) {
                errback(NET4CXX_MAKE_EXCEPTION_PTR(CancelledError, ""));
            }
        } else if (auto result = _result.asDeferred()){
            result->cancel();
        }
    }
protected:
    class Continuation {
    public:
        Continuation() = default;

        explicit Continuation(std::shared_ptr<Deferred<ValueT>> deferred)
                : _deferred(std::move(deferred)) {

        }

        std::shared_ptr<Deferred<ValueT>> getDeferred() const {
            return _deferred;
        }

        void operator()(DeferredValue<ValueT>&) {

        }
    protected:
        std::shared_ptr<Deferred<ValueT>> _deferred;
    };

    class RunningGuard {
    public:
        explicit RunningGuard(std::shared_ptr<Deferred<ValueT>> deferred)
                : _deferred(std::move(deferred)) {
            _deferred->_runningCallbacks = true;
        }

        ~RunningGuard() {
            _deferred->_runningCallbacks = false;
        }
    protected:
        std::shared_ptr<Deferred<ValueT>> _deferred;
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

    void runCallbacks() {
        if (_runningCallbacks) {
            return;
        }

        std::vector<std::shared_ptr<Deferred<ValueT>>> chain = {this->shared_from_this()};
        std::shared_ptr<Deferred<ValueT>> current;
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

                if (Continuation *cb = callback.template target<Continuation>()) {
                    auto chainee = cb->getDeferred();
                    chainee->_result = std::move(current->_result);
                    current->_result = DeferredNull;

                    --chainee->_paused;
                    chain.emplace_back(std::move(chainee));
                    finished = false;
                    break;
                }

                try {
                    RunningGuard guard(current);
                    callback(current->_result);
                    NET4CXX_ASSERT(current->_result.asDeferred() != current);
                } catch (...) {
                    current->_result = std::current_exception();
                }

                if (current->_result.isDeferred()) {
                    auto resultDeferred = current->_result.asDeferred();
                    NET4CXX_ASSERT(resultDeferred);
                    auto &resultResult = current->_result.asDeferred()->_result;

                    if (resultResult.isEmpty() || resultResult.isDeferred() || resultDeferred->_paused > 0) {
                        current->pause();
                        current->_chainTo = resultDeferred;
                        resultDeferred->_callbacks.emplace_back(current->continuation());
                    } else {
                        current->_result = resultResult;
                        resultDeferred->_result = DeferredNull;
                    }
                }
            }

            if (finished) {
                chain.pop_back();
            }
        }
    }

    bool _called{false};
    bool _suppressAlreadyCalled{false};
    bool _runningCallbacks{false};
    int _paused{0};
    std::weak_ptr<Deferred<ValueT>> _chainTo;
    CallbacksList _callbacks;
    CancellerType _canceller;
    DeferredValue<ValueT> _result{DeferredEmpty};
};


template <typename ValueT>
using DeferredPtr = std::shared_ptr<Deferred<ValueT>>;

template <typename ValueT>
DeferredPtr<ValueT> makeDeferred() {
    return std::make_shared<Deferred<ValueT>>();
}

template <typename ValueT, typename ResultT>
DeferredPtr<ValueT> succeedDeferred(ResultT &&result) {
    auto d = makeDeferred<ValueT>();
    d->callback(std::forward<ResultT>(result));
    return d;
}

template <typename ValueT>
DeferredPtr<ValueT> failDeferred(std::exception_ptr error= nullptr) {
    auto d = makeDeferred<ValueT>();
    d->errback(error);
    return d;
};

template <typename ValueT, typename CallableT, typename... Args>
DeferredPtr<ValueT> executeDeferrred(CallableT &&callable, Args&&... args) {
    using ResultT = typename std::result_of<CallableT>::type;
    try {
        ResultT result = callable(std::forward<Args>(args)...);
        return succeedDeferred<ValueT>(std::move(result));
    } catch (...) {
        return failDeferred<ValueT>();
    }
};

template <typename ValueT, typename CallableT, typename... Args>
DeferredPtr<ValueT> _maybeDeferred(Type2Type<DeferredPtr<ValueT>> ignore, CallableT &&callable, Args&&... args) {
    try {
        DeferredPtr<ValueT> result = callable(std::forward<Args>(args)...);
        return result;
    } catch (...) {
        return failDeferred<ValueT>();
    }
};

template <typename ValueT, typename CallableT, typename... Args>
DeferredPtr<ValueT> _maybeDeferred(Type2Type<DeferredValue<ValueT>> ignore, CallableT &&callable, Args&&... args) {
    DeferredValue<ValueT> result{DeferredEmpty};
    try {
        result = callable(std::forward<Args>(args)...);
    } catch (...) {
        return failDeferred<ValueT>();
    }
    if (result.isDeferred()) {
        return result.asDeferred();
    } else if (result.isError()) {
        return failDeferred(result.asError());
    }
    return succeedDeferred(std::move(result));
};

template <typename ValueT, typename ResultT, typename CallableT, typename... Args>
DeferredPtr<ValueT> _maybeDeferred(Type2Type<ResultT> &&ignore, CallableT &&callable, Args&&... args) {
    try {
        ResultT result = callable(std::forward<Args>(args)...);
        return succeedDeferred<ValueT>(std::move(result));
    } catch (...) {
        return failDeferred<ValueT>();
    }
};

template <typename ValueT, typename CallableT, typename... Args>
DeferredPtr<ValueT> maybeDeferred(CallableT &&callable, Args&&... args) {
    using ResultT = typename std::result_of<CallableT>::type;
    return _maybeDeferred(Type2Type<ResultT>(), std::forward<CallableT>(callable), std::forward<Args>(args)...);
};

NS_END

#endif //NET4CXX_CORE_NETWORK_DEFER_H
