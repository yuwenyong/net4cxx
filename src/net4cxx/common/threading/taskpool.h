//
// Created by yuwenyong.vincent on 2019-01-06.
//

#ifndef NET4CXX_COMMON_THREADING_TASKPOOL_H
#define NET4CXX_COMMON_THREADING_TASKPOOL_H

#include "net4cxx/common/common.h"
#include <future>
#include <thread>
#include <boost/intrusive/list.hpp>
#include <boost/thread/future.hpp>

NS_BEGIN


class NET4CXX_COMMON_API TaskPool {
public:
    class TaskBase: public boost::intrusive::list_base_hook<> {
    public:
        virtual void execute() = 0;
        virtual ~TaskBase() = default;
    };

    using TaskList = boost::intrusive::list<TaskBase, boost::intrusive::constant_time_size<false>>;

    ~TaskPool() {
        terminate();
        wait();
        _taskList.clear_and_dispose(TaskDisposer());
    }

    bool start() {
        return start(std::thread::hardware_concurrency());
    }

    bool start(size_t threadCount);

    template <typename FuncT>
    std::future<typename std::result_of<FuncT()>::type> submit(FuncT &&func) {
        using ResultType = typename std::result_of<FuncT()>::type;
        std::packaged_task<ResultType ()> task(std::forward<FuncT>(func));
        std::future<ResultType > res(task.get_future());
        push(makeTask(std::move(task)));
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_mut);
        return _taskList.empty();
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

    void wait() {
        for (auto &thread: _threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        _threads.clear();
    }
protected:
    template <typename FuncT>
    class Task: public TaskBase {
    public:
        explicit Task(FuncT &&func): _func(std::forward<FuncT>(func)) {}

        void execute() override {
            _func();
        }
    protected:
        FuncT _func;
    };

    struct TaskDisposer {
        void operator()(TaskBase *task) {
            delete task;
        }
    };

    void process();

    std::unique_ptr<TaskBase> pop();

    void push(std::unique_ptr<TaskBase> &&task);

    template <typename FuncT>
    static std::unique_ptr<TaskBase> makeTask(FuncT &&func) {
        return std::make_unique<Task<FuncT>>(std::forward<FuncT>(func));
    }

    mutable std::mutex _mut;
    std::condition_variable _cond;
    TaskList _taskList;
    bool _stopped{false};
    bool _terminated{false};
    std::vector<std::thread> _threads;
};


template <typename ValueT>
bool isReady(const std::future<ValueT> &fut) {
    return fut.wait_for(std::chrono::milliseconds::zero()) == std::future_status::ready;
}


NS_END

#endif //NET4CXX_COMMON_THREADING_TASKPOOL_H
