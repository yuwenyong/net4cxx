//
// Created by yuwenyong on 17-9-13.
//

#ifndef NET4CXX_COMMON_DEBUGGING_ASSERT_H
#define NET4CXX_COMMON_DEBUGGING_ASSERT_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/utilities/errors.h"
#include "net4cxx/common/utilities/strutil.h"


NS_BEGIN

class NET4CXX_COMMON_API AssertionError: public Exception {
public:
    AssertionError(const char *file, int line, const char *func, const std::string &message={}, std::string extra={})
            : Exception(file, line, func, message, {})
            , _extra(std::move(extra)) {
#ifndef NET4CXX_NDEBUG
        _backtrace = boost::lexical_cast<std::string>(boost::stacktrace::stacktrace(2, DEFAULT_STACKTRACE_MAX_DEPTH));
#endif
    }

    const char *what() const noexcept override;

    const char *getTypeName() const override {
        return "AssertionError";
    }
protected:
    std::string _extra;
};


class NET4CXX_COMMON_API Assert {
public:
    static void assertHandler(const char *file, int line, const char *function, const char *message) {
        throw AssertionError(file, line, function, message);
    }

    template <typename... Args>
    static void assertHandler(const char *file, int line, const char *function, const char *message, const char *format,
                              Args&&... args) {
        std::string error = StrUtil::format(format, std::forward<Args>(args)...);
        throw AssertionError(file, line, function, message, std::move(error));
    }
};

#if COMPILER == COMPILER_MICROSOFT
#define ASSERT_BEGIN __pragma(warning(push)) __pragma(warning(disable: 4127))
#define ASSERT_END __pragma(warning(pop))
#else
#define ASSERT_BEGIN
#define ASSERT_END
#endif

NS_END

#define NET4CXX_ASSERT_EX(cond, ...) ASSERT_BEGIN do { if (!(cond)) net4cxx::Assert::assertHandler(__FILE__, __LINE__, __FUNCTION__, #cond, ##__VA_ARGS__); } while(0) ASSERT_END


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
