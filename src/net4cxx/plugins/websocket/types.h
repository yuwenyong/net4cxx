//
// Created by yuwenyong.vincent on 2018/6/16.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_TYPES_H
#define NET4CXX_PLUGINS_WEBSOCKET_TYPES_H

#include "net4cxx/plugins/websocket/base.h"
#include "net4cxx/plugins/websocket/compress.h"

NS_BEGIN

class NET4CXX_COMMON_API ConnectionRequest {
public:
    ConnectionRequest(std::string peer, StringMap headers, std::string host, std::string path, QueryArgListMap params,
                      int version, std::string origin, StringVector protocols, WebSocketExtensionList extensions)
            : _peer(std::move(peer))
            , _headers(std::move(headers))
            , _host(std::move(host))
            , _path(std::move(path))
            , _params(std::move(params))
            , _version(version)
            , _origin(std::move(origin))
            , _protocols(std::move(protocols))
            , _extensions(std::move(extensions)) {

    }

    const std::string& getPeer() const {
        return _peer;
    }

    const StringMap& getHeaders() const {
        return _headers;
    }

    const std::string& getHost() const {
        return _host;
    }

    const std::string& getPath() const {
        return _path;
    }

    const QueryArgListMap& getParams() const {
        return _params;
    }

    int getVersion() const {
        return _version;
    }

    const std::string& getOrigin() const {
        return _origin;
    }

    const StringVector& getProtocols() const {
        return _protocols;
    }

    const WebSocketExtensionList& getExtensions() const {
        return _extensions;
    }
protected:
    std::string _peer;
    StringMap _headers;
    std::string _host;
    std::string _path;
    QueryArgListMap _params;
    int _version;
    std::string _origin;
    StringVector _protocols;
    WebSocketExtensionList _extensions;
};


class NET4CXX_COMMON_API ConnectionResponse {
public:
    ConnectionResponse(std::string peer, StringMap headers, int version, std::string protocol,
                       std::vector<PerMessageCompressPtr> extensions)
            : _peer(std::move(peer))
            , _headers(std::move(headers))
            , _version(version)
            , _protocol(std::move(protocol))
            , _extensions(std::move(extensions)) {

    }

    const std::string& getPeer() const {
        return _peer;
    }

    const StringMap& getHeaders() const {
        return _headers;
    }

    int getVersion() const {
        return _version;
    }

    const std::string& getProtocol() const {
        return _protocol;
    }

    const std::vector<PerMessageCompressPtr>& getExtensions() const {
        return _extensions;
    }
protected:
    std::string _peer;
    StringMap _headers;
    int _version;
    std::string _protocol;
    std::vector<PerMessageCompressPtr> _extensions;
};

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_TYPES_H
