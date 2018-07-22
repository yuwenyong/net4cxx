//
// Created by yuwenyong on 18-1-11.
//

#include "net4cxx/plugins/websocket/base.h"
#include "net4cxx/common/httputils/httplib.h"


NS_BEGIN


std::string ConnectionDeny::getReason() const {
    auto reason = boost::get_error_info<errinfo_http_reason>(*this);
    if (reason) {
        return *reason;
    }
    auto code = getCode();
    auto iter = HTTP_STATUS_CODES.find(code);
    if (iter != HTTP_STATUS_CODES.end()) {
        return iter->second;
    }
    return "Unknown";
}

StringVector ConnectionDeny::getCustomErrorInfo() const {
    StringVector errorInfo;
    errorInfo.emplace_back(StrUtil::format("HTTP %d: %s", getCode(), getReason()));
    return errorInfo;
}

NS_END