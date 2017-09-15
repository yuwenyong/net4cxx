//
// Created by yuwenyong on 17-9-15.
//

#ifndef NET4CXX_COMMON_LOGGING_ATTRIBUTES_H
#define NET4CXX_COMMON_LOGGING_ATTRIBUTES_H

#include "net4cxx/common/common.h"
#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/utility/value_ref.hpp>
#include <boost/phoenix.hpp>


NS_BEGIN

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;


using Severity = logging::trivial::severity_level;
using StringLiteral = logging::string_literal;

#define SEVERITY_TRACE      logging::trivial::trace
#define SEVERITY_DEBUG      logging::trivial::debug
#define SEVERITY_INFO       logging::trivial::info
#define SEVERITY_WARN       logging::trivial::warning
#define SEVERITY_ERROR      logging::trivial::error
#define SEVERITY_FATAL      logging::trivial::fatal


BOOST_LOG_ATTRIBUTE_KEYWORD(attr_severity, "Severity", Severity)
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_channel, "Channel", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_timestamp, "TimeStamp", DateTime)
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_file, "File", StringLiteral)
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_line, "Line", size_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_func, "Func", StringLiteral)


class TimeStampFormatterFactory: public logging::basic_formatter_factory<char, DateTime> {
public:
    formatter_type create_formatter(const logging::attribute_name &name, const args_map &args) {
        auto it = args.find("format");
        if (it != args.end()) {
            return expr::stream << expr::format_date_time<DateTime>(expr::attr<DateTime>(name), it->second);
        } else {
            return expr::stream << expr::attr<DateTime>(name);
        }
    }
};


class UptimeFormatterFactory: public logging::basic_formatter_factory<char, Time> {
public:
    formatter_type create_formatter(const logging::attribute_name &name, const args_map &args) {
        auto it = args.find("format");
        if (it != args.end()) {
            return expr::stream << expr::format_date_time<Time>(expr::attr<Time>(name), it->second);
        } else {
            return expr::stream << expr::attr<Time>(name);
        }
    }
};


class ChannelFilterFactory: public logging::filter_factory<char> {
public:
    logging::filter on_custom_relation(logging::attribute_name const& name, string_type const& rel,
                                       string_type const& arg) {
        if (rel == "child_of") {
            return boost::phoenix::bind(&childOf, expr::attr<string_type>(name), arg);
        }
        throw std::runtime_error("Unsupported filter relation: " + rel);
    }

    static bool childOf(const logging::value_ref<string_type> &channel, const string_type &r) {
        return isChildOf(channel.get(), r);
    }

    static bool isChildOf(const string_type &channel, const string_type &r);
};


NS_END


#endif //NET4CXX_COMMON_LOGGING_ATTRIBUTES_H
