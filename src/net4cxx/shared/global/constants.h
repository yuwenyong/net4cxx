//
// Created by yuwenyong.vincent on 2018/7/14.
//

#ifndef NET4CXX_SHARED_GLOBAL_CONSTANTS_H
#define NET4CXX_SHARED_GLOBAL_CONSTANTS_H

#include "net4cxx/common/common.h"

NS_BEGIN

class NET4CXX_COMMON_API WatchKeys {
public:
    static const char *TCPServerConnectionCount;
    static const char *TCPListenerCount;
    static const char *TCPClientConnectionCount;
    static const char *TCPConnectorCount;

    static const char *SSLServerConnectionCount;
    static const char *SSLListenerCount;
    static const char *SSLClientConnectionCount;
    static const char *SSLConnectorCount;

    static const char *UNIXServerConnectionCount;
    static const char *UNIXListenerCount;
    static const char *UNIXClientConnectionCount;
    static const char *UNIXConnectorCount;

    static const char *UDPConnectionCount;
    static const char *UNIXDatagramConnectionCount;

    static const char *DeferredCount;


    static const char *WebSocketServerProtocolCount;
    static const char *WebSocketClientProtocolCount;

    static const char *PeriodicCallbackCount;
    static const char *IOStreamCount;
    static const char *SSLIOStreamCount;
    static const char *HTTPClientCount;
    static const char *HTTPClientConnectionCount;
    static const char *HTTPConnectionCount;
    static const char *HTTPServerRequestCount;
    static const char *RequestHandlerCount;
};


NS_END

#endif //NET4CXX_SHARED_GLOBAL_CONSTANTS_H
