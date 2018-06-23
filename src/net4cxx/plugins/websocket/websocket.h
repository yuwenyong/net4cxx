//
// Created by yuwenyong.vincent on 2018/6/17.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_WEBSOCKET_H
#define NET4CXX_PLUGINS_WEBSOCKET_WEBSOCKET_H

#include "net4cxx/plugins/websocket/base.h"
#include "net4cxx/plugins/websocket/protocol.h"


NS_BEGIN

NET4CXX_COMMON_API ConnectorPtr connectWS(Reactor *reactor, std::shared_ptr<WebSocketClientFactory> factory,
                                          SSLOptionPtr sslOption=nullptr, double timeout=30.0,
                                          const Address &bindAddress={});

NET4CXX_COMMON_API ListenerPtr listenWS(Reactor *reactor, std::shared_ptr<WebSocketServerFactory> factory,
                                        SSLOptionPtr sslOption=nullptr, const std::string &interface={});

NS_END


#endif //NET4CXX_PLUGINS_WEBSOCKET_WEBSOCKET_H
