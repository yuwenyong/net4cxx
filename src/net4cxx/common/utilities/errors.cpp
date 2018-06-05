//
// Created by yuwenyong on 17-9-12.
//

#include "net4cxx/common/utilities/errors.h"


NS_BEGIN

const char* Exception::what() const noexcept {
    if (_what.empty()) {
        std::stringstream ss;
        ss << *boost::get_error_info<boost::throw_file>(*this);
        ss << ':';
        ss << *boost::get_error_info<boost::throw_line>(*this);
        ss << " in ";
        ss << *boost::get_error_info<boost::throw_function>(*this);
        ss << ' ';
        ss << getTypeName();
        ss << "\n\t";
        ss << *boost::get_error_info<errinfo_message>(*this);
        if (boost::get_error_info<errinfo_stack_trace>(*this)) {
            ss << "\nBacktrace:\n";
            ss << *boost::get_error_info<errinfo_stack_trace>(*this);
        }
        _what = ss.str();
    }
    return _what.c_str();
}

NS_END