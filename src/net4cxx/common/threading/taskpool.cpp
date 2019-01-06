//
// Created by yuwenyong.vincent on 2019-01-06.
//

#include "net4cxx/common/threading/taskpool.h"


NS_BEGIN

bool TaskPool::start(size_t threadCount) {
    if (threadCount == 0) {
        return false;
    }
    if (!_threads.empty()) {
        return false;
    }
    if (_stopped || _terminated) {
        return false;
    }
    for (size_t i = 0; i != threadCount; ++i) {
        _threads.emplace_back(std::thread([this](){
            process();
        }));
    }
    return true;
}

void TaskPool::process() {
    std::unique_ptr<TaskBase> task;
    while (true) {
        task = pop();
        if (!task) {
            break;
        }
        task->execute();
        task.reset();
    }
}

std::unique_ptr<TaskPool::TaskBase> TaskPool::pop() {
    std::unique_ptr<TaskBase> res;
    std::unique_lock<std::mutex> lock(_mut);
    do {
        if (_terminated) {
            break;
        } else if (!_taskList.empty()) {
            res.reset(&_taskList.front());
            _taskList.pop_front();
            break;
        } else if (_stopped) {
            break;
        }
        _cond.wait(lock);
    } while (true);
    return res;
}

void TaskPool::push(std::unique_ptr<TaskBase> &&task) {
    std::lock_guard<std::mutex> lock(_mut);
    _taskList.push_back(*task.release());
    _cond.notify_one();
}

NS_END