//
// Created by yuwenyong.vincent on 2019-01-06.
//

#ifndef NET4CXX_COMMON_THREADING_CONCURRENTQUEUE_H
#define NET4CXX_COMMON_THREADING_CONCURRENTQUEUE_H

#include "net4cxx/common/common.h"
#include <condition_variable>
#include <mutex>
#include <thread>


NS_BEGIN


template <typename ValueT>
class ConcurrentQueue {
public:
    bool empty() const {
        std::lock_guard<std::mutex> lock(_mut);
        return _queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(_mut);
        return _queue.size();
    }

    bool enqueue(ValueT &&val) {
        std::lock_guard<std::mutex> lock(_mut);
        if (_terminated || _stopped) {
            return false;
        }
        _queue.emplace_back(std::forward<ValueT>(val));
        _cond.notify_one();
        return true;
    }

    bool dequeue(ValueT &val) {
        bool res{false};
        std::unique_lock<std::mutex> lock(_mut);
        do {
            if (_terminated) {
                break;
            } else if (!_queue.empty()) {
                val = std::move(_queue.front());
                _queue.pop_front();
                res = true;
                break;
            } else if (_stopped) {
                break;
            }
            _cond.wait(lock);
        } while (true);
        return res;
    }

    std::shared_ptr<ValueT> dequeue() {
        std::shared_ptr<ValueT> res;
        std::unique_lock<std::mutex> lock(_mut);
        do {
            if (_terminated) {
                break;
            } else if (!_queue.empty()) {
                res = std::make_shared<ValueT>(std::move(_queue.front()));
                _queue.pop_front();
                break;
            } else if (_stopped) {
                break;
            }
            _cond.wait(lock);
        } while (true);
        return res;
    }

    bool tryDequeue(ValueT &val) {
        std::lock_guard<std::mutex> lock(_mut);
        if (_terminated) {
            return false;
        }
        if (!_queue.empty()) {
            val = std::move(_queue.front());
            _queue.pop_front();
            return true;
        }
        return false;
    }

    std::shared_ptr<ValueT> tryDequeue() {
        std::lock_guard<std::mutex> lock(_mut);
        if (_terminated) {
            return nullptr;
        }
        if (!_queue.empty()) {
            auto res = std::make_shared<ValueT>(std::move(_queue.front()));
            _queue.pop_front();
            return res;
        }
        return nullptr;
    }

    void stop() {
        std::lock_guard<std::mutex> lock(_mut);
        _stopped = true;
        _cond.notify_all();
    }

    bool stopped() const {
        std::lock_guard<std::mutex> lock(_mut);
        return _stopped;
    }

    void terminate() {
        std::lock_guard<std::mutex> lock(_mut);
        _terminated = true;
        _cond.notify_all();
    }

    bool terminated() const {
        std::lock_guard<std::mutex> lock(_mut);
        return _terminated;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(_mut);
        _stopped = false;
        _terminated = false;
    }
protected:
    mutable std::mutex _mut;
    std::condition_variable _cond;
    std::deque<ValueT> _queue;
    bool _stopped{false};
    bool _terminated{false};
};

NS_END

#endif //NET4CXX_COMMON_THREADING_CONCURRENTQUEUE_H
