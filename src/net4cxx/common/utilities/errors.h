//
// Created by yuwenyong on 17-9-12.
//

#ifndef NET4CXX_COMMON_UTILITIES_ERRORS_H
#define NET4CXX_COMMON_UTILITIES_ERRORS_H

#include "net4cxx/common/common.h"
#include <exception>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/stacktrace.hpp>

NS_BEGIN

struct tagException {

};

class NET4CXX_COMMON_API Exception: public std::runtime_error {
public:
    Exception(const char *file, int line, const char *func, const std::string &message={})
            : Exception(file, line, func, message, 1) {

    }

    const char *what() const noexcept override;

    virtual const char *getTypeName() const {
        return "Exception";
    }
protected:
    Exception(const char *file, int line, const char *func, const std::string &message, size_t skipFrames)
            : Exception(file, line, func, message, ++skipFrames, tagException{}) {

    }

    Exception(const char *file, int line, const char *func, const std::string &message, size_t skipFrames,
              tagException);

    const char *_file{nullptr};
    int _line{0};
    const char *_func{nullptr};
    std::string _backtrace;
    mutable std::string _what;
};


#define NET4CXX_DECLARE_EXCEPTION(Exception, ParentException) \
class NET4CXX_COMMON_API Exception: public ParentException { \
public: \
    Exception(const char *file, int line, const char *func, const std::string &message={}) \
            : Exception(file, line, func, message, 1) {} \
    const char *getTypeName() const override { \
        return #Exception; \
    } \
protected: \
    Exception(const char *file, int line, const char *func, const std::string &message, size_t skipFrames) \
            : ParentException(file, line, func, message, ++skipFrames) {} \
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


#define NET4CXX_EXCEPTION(Exception, ...)   Exception(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define NET4CXX_EXCEPTION_PTR(Exception, ...)   std::make_exception_ptr(NET4CXX_EXCEPTION(Exception, ##__VA_ARGS__))
#define NET4CXX_THROW_EXCEPTION(Exception, ...) throw NET4CXX_EXCEPTION(Exception, ##__VA_ARGS__)

NS_END

#endif //NET4CXX_COMMON_UTILITIES_ERRORS_H
