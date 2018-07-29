//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/common/debugging/assert.h"

NS_BEGIN

StringVector AssertionError::getCustomErrorInfo() const {
    StringVector errorInfo;
    auto expr = boost::get_error_info<errinfo_assert_expr>(*this);
    if (expr) {
        errorInfo.emplace_back(StrUtil::format("ASSERTION FAILED:%s", *expr));
    }
    return errorInfo;
}

NS_END