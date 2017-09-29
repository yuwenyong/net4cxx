//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/protocol.h"


NS_BEGIN


void Factory::doStart() {
    if (!_numPorts) {
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
        stopFactory();
    }
}

void Factory::startFactory() {

}

void Factory::stopFactory() {

}


NS_END