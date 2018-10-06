//
// Created by yuwenyong.vincent on 2018/7/16.
//

#ifndef NET4CXX_SHARED_GLOBAL_ERRORINFO_H
#define NET4CXX_SHARED_GLOBAL_ERRORINFO_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/utilities/errors.h"

NS_BEGIN


typedef boost::error_info<struct errinfo_http_code_, int> errinfo_http_code;

typedef boost::error_info<struct errinfo_http_reason_, std::string> errinfo_http_reason;

typedef boost::error_info<struct errinfo_arg_name_, std::string> errinfo_arg_name;

NS_END

#endif //NET4CXX_SHARED_GLOBAL_ERRORINFO_H
