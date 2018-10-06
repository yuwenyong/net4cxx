//
// Created by yuwenyong on 18-1-11.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_BASE_H
#define NET4CXX_PLUGINS_WEBSOCKET_BASE_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/httputils/urlparse.h"
#include "net4cxx/common/utilities/errors.h"
#include "net4cxx/common/utilities/strutil.h"
#include "net4cxx/shared/global/errorinfo.h"
#include "net4cxx/shared/global/loggers.h"


NS_BEGIN

using WebSocketMask = std::array<Byte, 4>;
using WebSocketExtensionParams = std::map<std::string, std::vector<boost::optional<std::string>>>;
using WebSocketExtensionList = std::vector<std::pair<std::string, WebSocketExtensionParams>>;
using WebSocketHeaders = std::map<std::string, std::vector<std::string>>;


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

    int getCode() const {
        auto code = boost::get_error_info<errinfo_http_code>(*this);
        NET4CXX_ASSERT(code != nullptr);
        return *code;
    }

    std::string getReason() const;
protected:
    const char *getTypeName() const override {
        return "ConnectionDeny";
    }

    StringVector getCustomErrorInfo() const override;
};


NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_BASE_H
