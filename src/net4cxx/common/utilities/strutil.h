//
// Created by yuwenyong on 17-9-12.
//

#ifndef NET4CXX_COMMON_UTILITIES_STRUTIL_H
#define NET4CXX_COMMON_UTILITIES_STRUTIL_H

#include "net4cxx/common/common.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#ifdef HAS_CPP_FORMAT
#include "fmt/format.h"
#endif

NS_BEGIN

class NET4CXX_COMMON_API StrUtil {
public:
    using PartitionResult = std::tuple<std::string, std::string, std::string>;

    static void capitalizeInplace(std::string &s);

    static std::string capitalize(const std::string &s) {
        std::string result(s);
        capitalizeInplace(result);
        return result;
    }

    static size_t count(const std::string &s, char c, size_t start=0, size_t len=0);

    static size_t count(const std::string &s, const std::string &sub, size_t start=0, size_t len=0);

    static size_t count(const std::string &s, std::function<bool (char)> pred);

    static std::string format(const char *fmt) {
        return std::string(fmt);
    }

#ifdef HAS_CPP_FORMAT
    template<typename... Args>
    static std::string format(const char *fmt, Args&&... args) {
        return fmt::sprintf(fmt, std::forward<Args>(args)...);
    }
#else
    template <typename... Args>
    static std::string format(const char *fmt, Args&&... args) {
        boost::format formatter(fmt);
        format(formatter, std::forward<Args>(args)...);
        return formatter.str();
    }
#endif

    static PartitionResult partition(const std::string &s, const std::string &sep);

    static PartitionResult rpartition(const std::string &s, const std::string &sep);

    static StringVector split(const std::string &s, bool keepEmpty=true) {
        StringVector result;
        boost::split(result, s, boost::is_space(), keepEmpty?boost::token_compress_off:boost::token_compress_on);
        return result;
    }

    static StringVector split(const std::string &s, char delim, bool keepEmpty=true) {
        StringVector result;
        boost::split(result, s, [delim](char c) {
            return c == delim;
        }, keepEmpty?boost::token_compress_off:boost::token_compress_on);
        return result;
    }

    static StringVector split(const std::string &s, const std::string &delim, bool keepEmpty=true);

    static StringVector splitLines(const std::string &s, bool keepends=false);

    static std::string translate(const std::string &s, const std::array<char, 256> &table,
    const std::vector<char> &deleteChars);

    static std::string translate(const std::string &s, const std::array<char, 256> &table) {
        std::vector<char> deleteChars;
        return translate(s, table, deleteChars);
    }

    static std::string filter(const std::string &s, std::function<bool (char)> pred);
protected:
#ifndef HAS_CPP_FORMAT
    template <typename ValueT, typename... Args>
    static void format(boost::format &formatter, ValueT &&value, Args&&... args) {
        formatter % std::forward<T>(value);
        format(formatter, std::forward<Args>(args)...);
    }

    template <typename ValueT>
    static void format(boost::format &formatter, ValueT &&value) {
        formatter % std::forward<T>(value);
    }
#endif
};

NS_END

#endif //NET4CXX_COMMON_UTILITIES_STRUTIL_H
