//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/common/utilities/util.h"
#include <boost/date_time/c_local_time_adjustor.hpp>
#include "net4cxx/common/utilities/strutil.h"

NS_BEGIN


std::string TypeUtil::typeCast(Type2Type<std::string>, const StringMap &m) {
    std::stringstream ss;
    bool first = true;
    ss << "{";
    for (auto &kv: m) {
        if (first) {
            first = false;
        } else {
            ss << ", ";
        }
        ss << '\'' << kv.first << "\': \'" << kv.second << '\'';
    }
    ss << "}";
    return ss.str();
}


std::string DateTimeUtil::formatDate(const DateTime &timeval, bool localtime, bool usegmt) {
    DateTime now;
    std::string zone;
    if (localtime) {
        now = boost::date_time::c_local_adjustor<DateTime>::utc_to_local(timeval);
        auto offset = timeval - now;
        if (offset.is_negative()) {
            zone = "-";
            offset.invert_sign();
        } else {
            zone = "+";
        }
        zone += StrUtil::format("%02d%02d", offset.hours(), offset.minutes());
    } else {
        now = timeval;
        if (usegmt) {
            zone = "GMT";
        } else {
            zone = "-0000";
        }
    }
    const char *weekdayNames[] = {
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    const char *monthNames[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    const Date date = now.date();
    const Time time = now.time_of_day();
    return StrUtil::format("%s, %02d %s %04d %02d:%02d:%02d %s", weekdayNames[date.day_of_week().as_number()],
                           date.day(), monthNames[(size_t)date.month() - 1], date.year(), time.hours(), time.minutes(),
                           time.seconds(), zone.c_str());
}

std::string DateTimeUtil::formatUTCDate(const DateTime &ts) {
    static std::locale loc(std::cout.getloc(), new boost::posix_time::time_facet("%a, %d %b %Y %H:%M:%S GMT"));
    std::stringstream ss;
    ss.imbue(loc);
    ss << ts;
    return ss.str();
}

DateTime DateTimeUtil::parseUTCDate(const std::string &date) {
    static std::locale loc(std::cin.getloc(), new boost::posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S GMT"));
    std::istringstream ss(date);
    ss.imbue(loc);
    DateTime dt;
    ss >> dt;
    return dt;
}


std::string BinAscii::hexlify(const Byte *s, size_t len, bool reverse) {
    int init = 0;
    auto end = (int)len;
    int op = 1;
    if (reverse) {
        init = (int)len - 1;
        end = -1;
        op = -1;
    }
    std::ostringstream ss;
    for (int i = init; i != end; i += op) {
        char buffer[4];
        sprintf(buffer, "%02X", s[i]);
        ss << buffer;
    }
    return ss.str();
}

ByteArray BinAscii::unhexlify(const char *s, size_t len, bool reverse) {
    ByteArray out;
    if (len & 1u) {
        return out;
    }
    int init = 0;
    auto end = (int)len;
    int op = 1;
    if (reverse) {
        init = (int)len - 2;
        end = -2;
        op = -1;
    }
    for (int i = init; i != end; i += 2 * op) {
        char buffer[3] = { s[i], s[i + 1], '\0' };
        out.push_back((Byte)strtoul(buffer, nullptr, 16));
    }
    return out;
}

NS_END