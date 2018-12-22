//
// Created by yuwenyong.vincent on 2018/7/22.
//

#ifndef NET4CXX_PLUGINS_WEB_HTTPUTIL_H
#define NET4CXX_PLUGINS_WEB_HTTPUTIL_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/httputils/httplib.h"
#include "net4cxx/common/httputils/urlparse.h"
#include "net4cxx/common/utilities/util.h"
#include "net4cxx/shared/global/errorinfo.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(HTTPInputError, Exception);
NET4CXX_DECLARE_EXCEPTION(HTTPOutputError, Exception);


class NET4CXX_COMMON_API HTTPHeaders {
public:
    typedef std::pair<std::string, std::string> NameValueType;
    typedef std::map<std::string, StringVector> HeadersContainerType;
    typedef std::function<void (const std::string&, const std::string&)> CallbackType;

    class HTTPHeadersSetter {
    public:
        HTTPHeadersSetter(std::string *value, StringVector *values)
                : _value(value)
                , _values(values) {
        }

        HTTPHeadersSetter& operator=(const std::string &value) {
            *_value = value;
            *_values = {value, };
            return *this;
        }

        explicit operator std::string() const {
            return *_value;
        }
    protected:
        std::string *_value{nullptr};
        StringVector *_values{nullptr};
    };

    HTTPHeaders() = default;

    HTTPHeaders(std::initializer_list<NameValueType> nameValues) {
        update(nameValues);
    }

    explicit HTTPHeaders(const StringMap &nameValues) {
        update(nameValues);
    }

    void add(const std::string &name, const std::string &value);

    StringVector getList(const std::string &name) const;

    void getAll(const CallbackType &callback) const {
        for (auto &name: _asList) {
            for (auto &value: name.second) {
                callback(name.first, value);
            }
        }
    }

    void parseLine(const std::string &line);

    HTTPHeadersSetter operator[](const std::string &name) {
        std::string normName = HTTPHeaders::normalizeName(name);
        return {&_items[normName], &_asList[normName]};
    }

    bool has(const std::string &name) const {
        std::string normName = HTTPHeaders::normalizeName(name);
        return _items.find(name) != _items.end();
    }

    const std::string& at(const std::string &name) const {
        return _items.at(HTTPHeaders::normalizeName(name));
    }

    void erase(const std::string &name);

    std::string get(const std::string &name, const std::string &defaultValue="") const;

    void update(const std::vector<NameValueType> &nameValues) {
        for(auto &nameValue: nameValues) {
            (*this)[nameValue.first] = nameValue.second;
        }
    }

    void update(const StringMap &nameValues) {
        for(auto &nameValue: nameValues) {
            (*this)[nameValue.first] = nameValue.second;
        }
    }

    void clear() {
        _items.clear();
        _asList.clear();
    }

    void parseLines(const std::string &headers);

    static std::shared_ptr<HTTPHeaders> parse(const std::string &headers) {
        auto h = std::make_shared<HTTPHeaders>();
        h->parseLines(headers);
        return h;
    }

    const StringMap& items() const {
        return _items;
    }
protected:
    static std::string normalizeName(const std::string &name);

    StringMap _items;
    HeadersContainerType _asList;
    std::string _lastKey;
    static const boost::regex _normalizedHeader;
};


NET4CXX_COMMON_API std::ostream& operator<<(std::ostream &os, const HTTPHeaders &headers);


class NET4CXX_COMMON_API HTTPFile {
public:
    HTTPFile(std::string fileName,
             std::string contentType,
             std::string body)
            : _fileName(std::move(fileName))
            , _contentType(std::move(contentType))
            , _body(std::move(body)) {

    }

    const std::string& getFileName() const {
        return _fileName;
    }

    const std::string& getContentType() const {
        return _contentType;
    }

    const std::string& getBody() const {
        return _body;
    }
protected:
    std::string _fileName;
    std::string _contentType;
    std::string _body;
};


using HTTPFileListMap = std::map<std::string, std::vector<HTTPFile>>;

using RequestStartLineBase = std::tuple<std::string, std::string, std::string>;

class NET4CXX_COMMON_API RequestStartLine: public RequestStartLineBase {
public:
    RequestStartLine()
            : RequestStartLineBase() {

    }

    RequestStartLine(std::string method,
                     std::string path,
                     std::string version)
            : RequestStartLineBase(std::move(method), std::move(path), std::move(version)) {

    }

    const std::string& getMethod() const {
        return std::get<0>(*this);
    }

    const std::string& getPath() const {
        return std::get<1>(*this);
    }

    const std::string& getVersion() const {
        return std::get<2>(*this);
    }
};


using ResponseStartLineBase = std::tuple<std::string, int, std::string>;


class NET4CXX_COMMON_API ResponseStartLine: public ResponseStartLineBase {
public:
    ResponseStartLine()
            : ResponseStartLineBase() {

    }

    ResponseStartLine(std::string version,
                      int code,
                      std::string reason)
            : ResponseStartLineBase(std::move(version), code, std::move(reason)) {

    }

    const std::string& getVersion() const {
        return std::get<0>(*this);
    }

    int getCode() const {
        return std::get<1>(*this);
    }

    const std::string& getReason() const {
        return std::get<2>(*this);
    }
};


class NET4CXX_COMMON_API HTTPUtil {
public:
    static std::string urlConcat(std::string url, const QueryArgMap &args);

    static std::string urlConcat(std::string url, const QueryArgList &args);

    static void parseBodyArguments(const std::string &contentType, const std::string &body, QueryArgListMap &arguments,
                                   HTTPFileListMap &files, const HTTPHeaders *headers=nullptr);

    static void parseMultipartFormData(std::string boundary, const std::string &data, QueryArgListMap &arguments,
                                       HTTPFileListMap &files);

    static std::string formatTimestamp(const DateTime &ts) {
        return DateTimeUtil::formatDate(ts, false, true);
    }

    static std::string formatTimestamp(time_t ts) {
        return formatTimestamp(boost::posix_time::from_time_t(ts));
    }

    static std::string formatTimestamp(const tm &ts) {
        return formatTimestamp(boost::posix_time::ptime_from_tm(ts));
    }

    static std::string getHTTPReason(int statusCode) {
        auto iter = HTTP_STATUS_CODES.find(statusCode);
        return iter != HTTP_STATUS_CODES.end() ? iter->second : "Unknown";
    }

    static RequestStartLine parseRequestStartLine(const std::string &line);

    static ResponseStartLine parseResponseStartLine(const std::string &line);

    static std::tuple<std::string, boost::optional<unsigned short>> splitHostAndPort(const std::string &netloc);

    static std::tuple<std::string, std::shared_ptr<HTTPHeaders>> parseHeaders(const char *data, size_t length);

    static StringMap parseCookie(const std::string &cookie);
protected:
    static StringVector parseParam(std::string s);

    static std::tuple<std::string, StringMap> parseHeader(const std::string &line);

    static std::string encodeHeader(const std::string &key, const StringMap &pdict);
};


class NET4CXX_COMMON_API HTTPError: public Exception {
public:
    int getCode() const {
        auto code = boost::get_error_info<errinfo_http_code>(*this);
        NET4CXX_ASSERT(code != nullptr);
        return *code;
    }

    std::string getReason() const;

    bool hasReason() const;
protected:
    const char *getTypeName() const override {
        return "HTTPError";
    }

    StringVector getCustomErrorInfo() const override;
};


NS_END

#endif //NET4CXX_PLUGINS_WEB_HTTPUTIL_H
