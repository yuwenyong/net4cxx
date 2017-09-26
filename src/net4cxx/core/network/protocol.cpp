//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/protocol.h"


NS_BEGIN

const char* Factory::logPrefix() const {
    return "Factory";
}

void Factory::doStart() {
    if (!_numPorts) {
        NET4CXX_INFO(gGenLog, "Starting factory {%s}", logPrefix());
        startFactory();
    }
    ++_numPorts;
}

void Factory::doStop() {
    if (!_numPorts) {
        return;
    }
    --_numPorts;
    if (!_numPorts) {
        NET4CXX_INFO(gGenLog, "Stop factory {%s}", logPrefix());
        stopFactory();
    }
}

void Factory::startFactory() {

}

void Factory::stopFactory() {

}


const char* Protocol::logPrefix() const {
    return "Protocol";
}

NS_END