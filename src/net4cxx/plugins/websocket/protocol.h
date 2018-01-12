//
// Created by yuwenyong on 18-1-11.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_PROTOCOL_H
#define NET4CXX_PLUGINS_WEBSOCKET_PROTOCOL_H

#include "net4cxx/plugins/websocket/base.h"
#include "net4cxx/core/network/protocol.h"

NS_BEGIN


class WebSocketOptions;


class NET4CXX_COMMON_API WebSocketProtocol: public Protocol {
public:
    enum class State {
        CLOSED = 0,
        CONNECTING,
        CLOSING,
        OPEN,
        PROXY_CONNECTING,
    };

    enum class SendState {
        GROUND = 0,
        MESSAGE_BEGIN,
        INSIDE_MESSAGE,
        INSIDE_MESSAGE_FRAME,
    };

    explicit WebSocketProtocol(WebSocketOptions *options)
            : _options(options) {

    }

    void connectionMade() override;

    void dataReceived(Byte *data, size_t length) override;

    void connectionLost(std::exception_ptr reason) override;

protected:
    std::string getPeerName() const;

    WebSocketOptions *_options{nullptr};
    std::string _peer{"<never connected>"};
    bool _tcpNoDelay{false};
};

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_PROTOCOL_H
