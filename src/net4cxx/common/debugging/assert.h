//
// Created by yuwenyong on 17-9-13.
//

#ifndef NET4CXX_COMMON_DEBUGGING_ASSERT_H
#define NET4CXX_COMMON_DEBUGGING_ASSERT_H

#include "net4cxx/common/common.h"
#include <boost/assert.hpp>
#include <boost/stacktrace.hpp>
#include "net4cxx/common/utilities/errors.h"


NS_BEGIN

typedef boost::error_info<struct errinfo_assert_expr_, const char *> errinfo_assert_expr;

class NET4CXX_COMMON_API AssertionError: public Exception {
protected:
    const char *getTypeName() const override {
        return "AssertionError";
    }

    StringVector getCustomErrorInfo() const override;
};

NS_END


#define NET4CXX_ASSERT_THROW(expr, msg, ...)   do { if (!(expr)) NET4CXX_THROW_EXCEPTION(AssertionError, msg, ##__VA_ARGS__) << net4cxx::errinfo_assert_expr(#expr); } while(0)

#define NET4CXX_ASSERT(expr)            BOOST_ASSERT(expr)
#define NET4CXX_ASSERT_MSG(expr, msg)   BOOST_ASSERT_MSG(expr, msg)
#define NET4CXX_VERIFY(expr)            BOOST_VERIFY(expr)
#define NET4CXX_VERIFY_MSG(expr, msg)   BOOST_VERIFY_MSG(expr, msg)

namespace boost {

inline void assertion_failed_msg(char const* expr, char const* msg, char const* function, char const *file, long line) {
    std::cerr << file << ": " << line << ": " << function << ": Assertion '" << expr << "' failed:  "
              << (msg ? msg : "<...>") << ".\n" << "Backtrace:\n" << boost::stacktrace::stacktrace() << '\n';
    std::abort();
}

inline void assertion_failed(char const* expr, char const* function, char const* file, long line) {
    ::boost::assertion_failed_msg(expr, nullptr, function, file, line);
}

}

#endif //NET4CXX_COMMON_DEBUGGING_ASSERT_H
