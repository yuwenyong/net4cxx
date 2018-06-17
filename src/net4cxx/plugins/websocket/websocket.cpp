//
// Created by yuwenyong.vincent on 2018/6/17.
//

#include "net4cxx/plugins/websocket/websocket.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN

ConnectorPtr connectWS(Reactor *reactor, std::shared_ptr<WebSocketClientFactory> factory, SSLOptionPtr sslOption,
                       double timeout, const Address &bindAddress) {
    if (factory->isSecure()) {
        if (!sslOption) {
            SSLParams params(false);
            sslOption = SSLOption::create(params);
        }
    }
    ConnectorPtr conn;
    if (factory->isSecure()) {
        conn = reactor->connectSSL(factory->getHost(), std::to_string(factory->getPort()), std::move(factory),
                                   std::move(sslOption), timeout, bindAddress);
    } else {
        conn = reactor->connectTCP(factory->getHost(), std::to_string(factory->getPort()), std::move(factory),
                                   timeout, bindAddress);
    }
    return conn;
}

ListenerPtr listenWS(Reactor *reactor, std::shared_ptr<WebSocketServerFactory> factory, SSLOptionPtr sslOption,
                     const std::string &interface) {
    ListenerPtr listener;
    if (factory->isSecure()) {
        if (!sslOption) {
            NET4CXX_THROW_EXCEPTION(Exception, "Secure WebSocket listen requested, but no SSL context given");
        }
        listener = reactor->listenSSL(std::to_string(factory->getPort()), std::move(factory), std::move(sslOption),
                                      interface);
    } else {
        listener = reactor->listenTCP(std::to_string(factory->getPort()), std::move(factory), interface);
    }
    return listener;
}

NS_END