//
// Created by yuwenyong.vincent on 2018-12-23.
//

#ifndef NET4CXX_COMMON_EMAIL_UTILS_H
#define NET4CXX_COMMON_EMAIL_UTILS_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/httputils/urlparse.h"

NS_BEGIN


class NET4CXX_COMMON_API EMailUtils {
public:
    static QueryArgList decodeParams(QueryArgList params);

    static std::string collapseRFC2231Value(const std::string &value) {
        return unquote(value);
    }

    static std::string decodeRFC2231(const std::string &s);

    static std::string unquote(std::string value);

    static std::string quote(std::string value);

    static const boost::regex rfc2231Continuation;
};

NS_END

#endif //NET4CXX_COMMON_EMAIL_UTILS_H
