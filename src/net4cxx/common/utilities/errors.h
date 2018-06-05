//
// Created by yuwenyong on 17-9-12.
//

#ifndef NET4CXX_COMMON_UTILITIES_ERRORS_H
#define NET4CXX_COMMON_UTILITIES_ERRORS_H

#include "net4cxx/common/common.h"
#include <exception>
#include <stdexcept>
#include <boost/exception/all.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/stacktrace.hpp>

NS_BEGIN


typedef boost::error_info<struct errinfo_message_,std::string> errinfo_message;
typedef boost::error_info<struct errinfo_stack_trace_, boost::stacktrace::stacktrace> errinfo_stack_trace;


struct NET4CXX_COMMON_API Exception: virtual std::exception, virtual boost::exception {
public:
    const char *what() const noexcept override;

    virtual const char *getTypeName() const {
        return "Exception";
    }

protected:
    mutable std::string _what;
};


#define NET4CXX_DECLARE_EXCEPTION(Exception, BaseException) \
class NET4CXX_COMMON_API Exception: public BaseException { \
public: \
    const char *getTypeName() const override { \
        return #Exception; \
    } \
}


NET4CXX_DECLARE_EXCEPTION(KeyError, Exception);
NET4CXX_DECLARE_EXCEPTION(IndexError, Exception);
NET4CXX_DECLARE_EXCEPTION(TypeError, Exception);
NET4CXX_DECLARE_EXCEPTION(ValueError, Exception);
NET4CXX_DECLARE_EXCEPTION(IllegalArguments, Exception);
NET4CXX_DECLARE_EXCEPTION(IOError, Exception);
NET4CXX_DECLARE_EXCEPTION(EOFError, Exception);
NET4CXX_DECLARE_EXCEPTION(DuplicateKey, Exception);
NET4CXX_DECLARE_EXCEPTION(NotFound, Exception);
NET4CXX_DECLARE_EXCEPTION(AlreadyExist, Exception);
NET4CXX_DECLARE_EXCEPTION(NotExist, Exception);
NET4CXX_DECLARE_EXCEPTION(MemoryError, Exception);
NET4CXX_DECLARE_EXCEPTION(NotImplementedError, Exception);
NET4CXX_DECLARE_EXCEPTION(TimeoutError, Exception);
NET4CXX_DECLARE_EXCEPTION(ParsingError, Exception);
NET4CXX_DECLARE_EXCEPTION(PermissionError, Exception);


#define NET4CXX_EXCEPTION(Exception, msg, ...)   Exception(##__VA_ARGS__) << \
    boost::throw_function(BOOST_THROW_EXCEPTION_CURRENT_FUNCTION) << \
    boost::throw_file(__FILE__) << \
    boost::throw_line((int)__LINE__) << \
    errinfo_stack_trace(boost::stacktrace::stacktrace()) << \
    errinfo_message(msg)

#define NET4CXX_EXCEPTION_PTR(Exception, ...)   std::make_exception_ptr(NET4CXX_EXCEPTION(Exception, ##__VA_ARGS__))

#define NET4CXX_THROW_EXCEPTION(Exception, ...) throw NET4CXX_EXCEPTION(Exception, ##__VA_ARGS__)

NS_END

#endif //NET4CXX_COMMON_UTILITIES_ERRORS_H
