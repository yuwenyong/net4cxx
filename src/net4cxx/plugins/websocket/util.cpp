//
// Created by yuwenyong on 18-1-11.
//

#include "net4cxx/plugins/websocket/util.h"


NS_BEGIN

void TrafficStats::reset() {
    _outgoingOctetsWireLevel = 0;
    _outgoingOctetsWebSocketLevel = 0;
    _outgoingOctetsAppLevel = 0;
    _outgoingWebSocketFrames = 0;
    _outgoingWebSocketMessages = 0;

    _incomingOctetsWireLevel = 0;
    _incomingOctetsWebSocketLevel = 0;
    _incomingOctetsAppLevel = 0;
    _incomingWebSocketFrames = 0;
    _incomingWebSocketMessages = 0;

    _preopenOutgoingOctetsWireLevel = 0;
    _preopenIncomingOctetsWireLevel = 0;
}

void WebSocketOptions::resetProtocolOptions() {

}

NS_END