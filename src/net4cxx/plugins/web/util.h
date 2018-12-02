//
// Created by yuwenyong.vincent on 2018/10/14.
//

#ifndef NET4CXX_PLUGINS_WEB_UTIL_H
#define NET4CXX_PLUGINS_WEB_UTIL_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/compress/zlib.h"
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/core/network/reactor.h"
#include "net4cxx/shared/global/constants.h"


NS_BEGIN


class NET4CXX_COMMON_API PeriodicCallback: public std::enable_shared_from_this<PeriodicCallback> {
public:
    using CallbackType = std::function<void ()>;

    template <typename CallbackT>
    PeriodicCallback(CallbackT &&callback, double callbackTime, Reactor *reactor= nullptr)
            : PeriodicCallback(
            std::forward<CallbackT>(callback),
            std::chrono::milliseconds(int64_t(callbackTime * 1000)),
            reactor) {

    }

    template <typename CallbackT>
    PeriodicCallback(CallbackT &&callback, const Duration &callbackTime, Reactor *reactor= nullptr)
            : _callback(std::forward<CallbackT>(callback))
            , _callbackTime(callbackTime)
            , _reactor(reactor ? reactor : Reactor::current()) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::PeriodicCallbackCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~PeriodicCallback() {
        NET4CXX_Watcher->dec(WatchKeys::PeriodicCallbackCount);
    }
#endif

    void start() {
        _running = true;
        scheduleNext();
    }

    void stop() {
        _running = false;
        if (!_timeout.cancelled()) {
            _timeout.cancel();
            _timeout.reset();
        }
    }

    template <typename ...Args>
    static std::shared_ptr<PeriodicCallback> create(Args&& ...args) {
        return std::make_shared<PeriodicCallback>(std::forward<Args>(args)...);
    }
protected:
    void run();

    void scheduleNext() {
        if (_running) {
            _timeout = _reactor->callLater(_callbackTime, std::bind(&PeriodicCallback::run, shared_from_this()));
        }
    }

    CallbackType _callback;
    Duration _callbackTime;
    Reactor *_reactor;
    bool _running{false};
    DelayedCall _timeout;
};

using PeriodicCallbackPtr = std::shared_ptr<PeriodicCallback>;


class NET4CXX_COMMON_API GzipDecompressor {
public:
    GzipDecompressor()
            : _decompressObj(16 + Zlib::maxWBits) {

    }

    ByteArray decompress(const Byte *data, size_t len, size_t maxLength=0) {
        return _decompressObj.decompress(data, len, maxLength);
    }

    ByteArray decompress(const ByteArray &data, size_t maxLength=0) {
        return _decompressObj.decompress(data, maxLength);
    }

    ByteArray decompress(const std::string &data, size_t maxLength=0) {
        return _decompressObj.decompress(data, maxLength);
    }

    std::string decompressToString(const Byte *data, size_t len, size_t maxLength=0) {
        return _decompressObj.decompressToString(data, len, maxLength);
    }

    std::string decompressToString(const ByteArray &data, size_t maxLength=0) {
        return _decompressObj.decompressToString(data, maxLength);
    }

    std::string decompressToString(const std::string &data, size_t maxLength=0) {
        return _decompressObj.decompressToString(data, maxLength);
    }

    ByteArray flush() {
        return _decompressObj.flush();
    }

    std::string flushToString() {
        return _decompressObj.flushToString();
    }

    const ByteArray& getUnconsumedTail() const {
        return _decompressObj.getUnconsumedTail();
    }
protected:
    DecompressObj _decompressObj;
};

NS_END

#endif //NET4CXX_PLUGINS_WEB_UTIL_H
