//
// Created by yuwenyong on 18-1-11.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_BASE_H
#define NET4CXX_PLUGINS_WEBSOCKET_BASE_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/common/httputils/urlparse.h"
#include "net4cxx/common/utilities/errors.h"
#include "net4cxx/common/utilities/strutil.h"


NS_BEGIN

using WebSocketMask = std::array<Byte, 4>;
using WebSocketExtensionParams = std::map<std::string, std::vector<boost::optional<std::string>>>;
using WebSocketExtensionList = std::vector<std::pair<std::string, WebSocketExtensionParams>>;
using WebSocketHeaders = std::map<std::string, std::vector<std::string>>;


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


class NET4CXX_COMMON_API ConnectionDeny: public Exception {
public:
    enum StatusCode {
        BAD_REQUEST = 400,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        NOT_ACCEPTABLE = 406,
        REQUEST_TIMEOUT = 408,
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501,
        SERVICE_UNAVAILABLE = 503
    };

    explicit ConnectionDeny(int code): _code(code) {}

    int getCode() const {
        return _code;
    }

    std::string getReason() const {
        return *boost::get_error_info<errinfo_message>(*this);
    }
protected:
    int _code;
};


NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_BASE_H
