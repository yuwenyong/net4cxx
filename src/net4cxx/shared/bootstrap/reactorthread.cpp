//
// Created by yuwenyong.vincent on 2019-02-10.
//

#include "net4cxx/shared/bootstrap/reactorthread.h"


NS_BEGIN

ReactorThread::~ReactorThread() {
    stop();
    wait();
}

bool ReactorThread::start(bool installSignalHandlers) {
    if (!_thread) {
        onPreInit();
        _thread = std::make_unique<std::thread>([this, installSignalHandlers](){
            onInit();
            _reactor.makeCurrent();
            onRun();
            _reactor.run(installSignalHandlers);
            onQuit();
        });
        return true;
    } else {
        return false;
    }
}

void ReactorThread::onPreInit() {

}

void ReactorThread::onInit() {

}

void ReactorThread::onRun() {

}

void ReactorThread::onQuit() {

}

NS_END