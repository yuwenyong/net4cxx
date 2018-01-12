//
// Created by yuwenyong on 18-1-11.
//

#include "net4cxx/plugins/websocket/protocol.h"


NS_BEGIN

void WebSocketProtocol::connectionMade() {
    _peer = getPeerName();
    NET4CXX_DEBUG(gGenLog, "Connection made to %s", _peer.c_str());
    setNoDelay(_tcpNoDelay);
}

void WebSocketProtocol::dataReceived(Byte *data, size_t length) {

}

void WebSocketProtocol::connectionLost(std::exception_ptr reason) {

}

std::string WebSocketProtocol::getPeerName() const {
    std::string res;
    auto address = getRemoteAddress();
    if (NetUtil::isValidIPv4(address)) {
        res = "tcp4:" + address + std::to_string(getRemotePort());
    } else if (NetUtil::isValidIPv6(address)) {
        res = "tcp6:" + address + std::to_string(getRemotePort());
    } else if (!address.empty()) {
        res = "unix:" + address;
    } else {
        res = "?:";
    }
    return res;
}

NS_END