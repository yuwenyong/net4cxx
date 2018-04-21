//
// Created by yuwenyong on 17-9-12.
//

#include "net4cxx/common/utilities/errors.h"


NS_BEGIN

const char* Exception::what() const noexcept {
    if (_what.empty()) {
        _what += _file;
        _what += ':';
        _what += std::to_string(_line);
        _what += " in ";
        _what += _func;
        _what += ' ';
        _what += getTypeName();
        _what += "\n\t";
        _what += std::runtime_error::what();
        if (!_backtrace.empty()) {
            _what += "\nBacktrace:\n";
            _what += _backtrace;
        }
    }
    return _what.c_str();
}

Exception::Exception(const char *file, int line, const char *func, const std::string &message, size_t skipFrames,
                     tagException)
        : runtime_error(message)
        , _file(file)
        , _line(line)
        , _func(func)  {
#ifdef NET4CXX_DEBUG
    _backtrace = boost::lexical_cast<std::string>(boost::stacktrace::stacktrace(2 + skipFrames,
                                                                                DEFAULT_STACKTRACE_MAX_DEPTH));
#endif
}


NS_END