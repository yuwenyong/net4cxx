//
// Created by yuwenyong.vincent on 2018/8/11.
//

#ifndef NET4CXX_PLUGINS_WEB_WEB_H
#define NET4CXX_PLUGINS_WEB_WEB_H

#include "net4cxx/common/common.h"
#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>
#include "net4cxx/common/compress/gzip.h"
#include "net4cxx/common/configuration/json.h"
#include "net4cxx/plugins/web/httpserver.h"


NS_BEGIN


class WebApp;
using WebAppPtr = std::shared_ptr<WebApp>;

class OutputTransform;
using OutputTransformPtr = std::shared_ptr<OutputTransform>;


class NET4CXX_COMMON_API MissingArgumentError: public HTTPError {
public:
    std::string getArgName() const {
        auto argName = boost::get_error_info<errinfo_arg_name>(*this);
        NET4CXX_ASSERT(argName != nullptr);
        return *argName;
    }

protected:
    const char* getTypeName() const override {
        return "MissingArgumentError";
    }

    StringVector getCustomErrorInfo() const override;
};


class NET4CXX_COMMON_API RequestHandler: public std::enable_shared_from_this<RequestHandler> {
public:
    typedef std::map<std::string, boost::any> ArgsType;
    typedef std::vector<OutputTransformPtr> TransformsType;
//    typedef std::vector<std::pair<std::string, std::string>> ListHeadersType;
    typedef std::map<std::string, boost::any> SettingsType;
    typedef boost::optional<SimpleCookie> CookiesType;
//    typedef boost::optional<std::vector<SimpleCookie>> NewCookiesType;
    typedef boost::property_tree::ptree SimpleJSONType;
    typedef std::function<void ()> FlushCallbackType;

    friend class WebApp;
    friend class RequestDispatcher;

    RequestHandler(WebAppPtr application, HTTPServerRequestPtr request)
            : _application(std::move(application))
            , _request(std::move(request)) {
        clear();
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::RequestHandlerCount);
#endif
    }

    virtual ~RequestHandler();

    void start(ArgsType &args);

    virtual void initialize(ArgsType &args);

    const SettingsType& getSettings() const;

    virtual DeferredPtr onHead(const StringVector &args);

    virtual DeferredPtr onGet(const StringVector &args);

    virtual DeferredPtr onPost(const StringVector &args);

    virtual DeferredPtr onDelete(const StringVector &args);

    virtual DeferredPtr onPatch(const StringVector &args);

    virtual DeferredPtr onPut(const StringVector &args);

    virtual DeferredPtr onOptions(const StringVector &args);

    virtual DeferredPtr prepare();

    virtual void onFinish();

    virtual void onConnectionClose();

    virtual bool hasStreamRequestBody() const;

    virtual void dataReceived(std::string data);

    void clear();

    virtual void setDefaultHeaders();

    void setStatus(int statusCode, const std::string &reason="");

    int getStatus() const {
        return _statusCode;
    }

    template <typename ValueT>
    void setHeader(const std::string &name, const ValueT &value) {
        _headers[name] = convertHeaderValue(value);
    }

    template <typename ValueT>
    void addHeader(const std::string &name, const ValueT &value) {
        _headers.add(name, convertHeaderValue(value));
    }

    void clearHeader(const std::string &name) {
        if (_headers.has(name)) {
            _headers.erase(name);
        }
    }

    bool hasArgument(const std::string &name) const {
        auto &arguments = _request->getArguments();
        return arguments.find(name) != arguments.end();
    }

    std::string getArgument(const std::string &name, const char *defaultValue= nullptr, bool strip= true) const {
        return getArgument(name, _request->getArguments(), defaultValue, strip);
    }

    StringVector getArguments(const std::string &name, bool strip= true) const {
        return getArguments(name, _request->getArguments(), strip);
    }

    bool hasBodyArgument(const std::string &name) const {
        auto &arguments = _request->getBodyArguments();
        return arguments.find(name) != arguments.end();
    }

    std::string getBodyArgument(const std::string &name, const char *defaultValue= nullptr, bool strip= true) const {
        return getArgument(name, _request->getBodyArguments(), defaultValue, strip);
    }

    StringVector getBodyArguments(const std::string &name, bool strip= true) const {
        return getArguments(name, _request->getBodyArguments(), strip);
    }

    bool hasQueryArgument(const std::string &name) const {
        auto &arguments = _request->getQueryArguments();
        return arguments.find(name) != arguments.end();
    }

    std::string getQueryArgument(const std::string &name, const char *defaultValue= nullptr, bool strip= true) const {
        return getArgument(name, _request->getQueryArguments(), defaultValue, strip);
    }

    StringVector getQueryArguments(const std::string &name, bool strip= true) const {
        return getArguments(name, _request->getQueryArguments(), strip);
    }

    const SimpleCookie& cookies() const {
        return _request->cookies();
    }

    std::string getCookie(const std::string &name, const std::string &defaultValue= "") const {
        if (cookies().has(name)) {
            return cookies().at(name).getValue();
        }
        return defaultValue;
    }

    void setCookie(const std::string &name, const std::string &value, const std::string &domain= {},
                   const DateTime &expires= {}, const std::string &path= "/",
                   boost::optional<int> expiresDays= boost::none, const StringMap &args= {});

    void clearCookie(const std::string &name, const std::string &path= "/", const std::string &domain= {}) {
        DateTime expires = boost::posix_time::second_clock::universal_time() - boost::gregorian::days(365);
        setCookie(name, "", domain, expires, path);
    }

    void clearAllCookies(const std::string &path= "/", const std::string &domain= {}) {
        cookies().getAll([this, &path, &domain](const std::string &name, const Morsel &morsel) {
            clearCookie(name, path, domain);
        });
    }

    void redirect(const std::string &url, bool permanent=false, boost::optional<int> status=boost::none);

    void write(std::string &&chunk) {
        NET4CXX_ASSERT(!_finished);
        _writeBuffer.emplace_back(std::move(chunk));
    }

    void write(const std::string &chunk) {
        NET4CXX_ASSERT(!_finished);
        _writeBuffer.emplace_back(chunk);
        write((const Byte *)chunk.data(), chunk.size());
    }

    void write(const Byte *chunk, size_t length) {
        write(std::string{(const char *)chunk, length});
    }

    void write(const char *chunk) {
        write(std::string{chunk});
    }

    void write(const ByteArray &chunk) {
        write(chunk.data(), chunk.size());
    }

    void write(const SimpleJSONType &chunk) {
        NET4CXX_ASSERT(!_finished);
        setHeader("Content-Type", "application/json; charset=UTF-8");
        write(JsonUtil::encode(chunk));
    }

    void write(const JsonValue &chunk) {
        NET4CXX_ASSERT(!_finished);
        setHeader("Content-Type", "application/json; charset=UTF-8");
        write(boost::lexical_cast<std::string>(chunk));
    }

    void write() {
        NET4CXX_ASSERT(!_finished);
    }

    void flush(bool includeFooters = false, FlushCallbackType callback = nullptr);

    template <typename... Args>
    void finish(Args&&... args) {
        write(std::forward<Args>(args)...);
        if (!_headersWritten) {
            const std::string &method = _request->getMethod();
            if (_statusCode == 200 && (method == "GET" || method == "HEAD") && !_headers.has("Etag")) {
                setEtagHeader();
                if (checkEtagHeader()) {
                    _writeBuffer.clear();
                    setStatus(304);
                }
            }
            if (_statusCode == 304) {
                NET4CXX_ASSERT_MSG(_writeBuffer.empty(), "Cannot send body with 304");
                clearHeadersFor304();
            } else if (!_headers.has("Content-Length")) {
                size_t contentLength = std::accumulate(_writeBuffer.begin(), _writeBuffer.end(), 0,
                                                       [](size_t lhs, const std::string &rhs) {
                                                           return lhs + rhs.size();
                                                       });
                setHeader("Content-Length", contentLength);
            }
        }
        auto connection = _request->getConnection();
        if (connection) {
            connection->setCloseCallback(nullptr);
        }
        flush(true);
        _request->finish();
        log();
        _finished = true;
        onFinish();
    }

    void sendError(int statusCode = 500, const std::exception_ptr &error = nullptr, std::string reason="");

    virtual void writeError(int statusCode, const std::exception_ptr &error);

    void requireSetting(const std::string &name, const std::string &feature="this feature");

    template <typename... Args>
    std::string reverseUrl(const std::string &name, Args&&... args);

    virtual std::string computeEtag() const;

    void setEtagHeader() {
        auto etag = computeEtag();
        if (!etag.empty()) {
            setHeader("Etag", etag);
        }
    }

    bool checkEtagHeader() const;

    std::shared_ptr<const HTTPServerRequest> getRequest() const {
        return _request;
    }

    std::shared_ptr<HTTPServerRequest> getRequest() {
        return _request;
    }

    std::shared_ptr<const WebApp> getApplication() const {
        return _application;
    }

    std::shared_ptr<WebApp> getApplication() {
        return _application;
    }

    std::shared_ptr<HTTPConnection> getConnection() {
        auto connection = _request->getConnection();
        if (!connection) {
            NET4CXX_THROW_EXCEPTION(StreamClosedError, "Connection already closed");
        }
        return connection;
    }

    std::shared_ptr<const HTTPConnection> getConnection() const {
        auto connection = _request->getConnection();
        if (!connection) {
            NET4CXX_THROW_EXCEPTION(StreamClosedError, "Connection already closed");
        }
        return connection;
    }

    template <typename SelfT>
    std::shared_ptr<SelfT> getSelf() {
        return std::static_pointer_cast<SelfT>(shared_from_this());
    }

    template <typename SelfT>
    std::shared_ptr<const SelfT> getSelf() const {
        return std::static_pointer_cast<const SelfT>(shared_from_this());
    }

    static const StringSet SUPPORTED_METHODS;
protected:
    std::string convertHeaderValue(const std::string &value) {
        if (boost::regex_search(value, _invalidHeaderCharRe)) {
            NET4CXX_THROW_EXCEPTION(ValueError, "Unsafe header value " + value);
        }
        return value;
    }

    std::string convertHeaderValue(const DateTime &value) {
        return convertHeaderValue(HTTPUtil::formatTimestamp(value));
    }

    template <typename ValueT>
    std::string convertHeaderValue(const ValueT &value) {
        return convertHeaderValue(value, std::is_integral<ValueT>{});
    }

    template <typename ValueT>
    std::string convertHeaderValue(const ValueT &value, std::true_type) {
        return std::to_string(value);
    }

    template <typename ValueT>
    std::string convertHeaderValue(const ValueT &value, std::false_type) {
        return convertHeaderValue(boost::lexical_cast<std::string>(value));
    }

    std::string getArgument(const std::string &name, const QueryArgListMap &source, const char *defaultValue= nullptr,
                            bool strip= true) const;

    StringVector getArguments(const std::string &name, const QueryArgListMap &source, bool strip= true) const;

    virtual void execute(TransformsType transforms, const StringVector &args);

    void whenComplete();

    void executeMethod();

    void executeFinish() {
        if (_autoFinish && !_finished) {
            finish();
        }
    }

    void log();

    std::string requestSummary() const {
        return _request->getMethod() + " " + _request->getURI() + " (" + _request->getRemoteIp() + ")";
    }

    void handleRequestException(const std::exception_ptr &error);

    virtual void logException(const std::exception_ptr &error);

    void clearHeadersFor304() {
        const StringVector headers = {"Allow", "Content-Encoding", "Content-Language",
                                      "Content-Length", "Content-MD5", "Content-Range",
                                      "Content-Type", "Last-Modified"};
        for (auto &h: headers) {
            clearHeader(h);
        }
    }

    WebAppPtr _application;
    HTTPServerRequestPtr _request;
    bool _headersWritten{false};
    bool _finished{false};
    bool _autoFinish{true};
    TransformsType _transforms;
    StringVector _pathArgs;
    HTTPHeaders _headers;
//    ListHeadersType _listHeaders;
    StringVector _writeBuffer;
    int _statusCode;
    CookiesType _newCookie;
    std::string _reason;

    static const boost::regex _removeControlCharsRegex;
    static const boost::regex _invalidHeaderCharRe;
};

using RequestHandlerPtr = std::shared_ptr<RequestHandler>;


class NET4CXX_COMMON_API ErrorHandler: public RequestHandler {
public:
    using RequestHandler::RequestHandler;

    void initialize(ArgsType &args) override;

    DeferredPtr prepare() override;
};


class NET4CXX_COMMON_API RedirectHandler: public RequestHandler {
public:
    using RequestHandler::RequestHandler;

    void initialize(ArgsType &args) override;

    DeferredPtr onGet(const StringVector &args) override;
protected:
    std::string _url;
    bool _permanent{true};
};


class NET4CXX_COMMON_API FallbackHandler: public RequestHandler {
public:
    typedef std::function<void (HTTPServerRequestPtr)> FallbackType;

    using RequestHandler::RequestHandler;

    void initialize(ArgsType &args) override;

    DeferredPtr prepare() override;
protected:
    FallbackType _fallback;
};


class NET4CXX_COMMON_API BasicRequestHandlerFactory {
public:
    typedef RequestHandler::ArgsType ArgsType;

    virtual ~BasicRequestHandlerFactory() = default;

    virtual const char * getName() const = 0;

    virtual RequestHandlerPtr create(WebAppPtr application, HTTPServerRequestPtr request, ArgsType &args) const =0;
protected:
    const char *_name{nullptr};
};

using RequestHandlerFactoryPtr = std::shared_ptr<BasicRequestHandlerFactory>;


template <typename RequestHandlerT>
class RequestHandlerFactory: public BasicRequestHandlerFactory {
public:
    const char * getName() const override {
        return typeid(RequestHandlerT).name();
    }

    RequestHandlerPtr create(WebAppPtr application, HTTPServerRequestPtr request, ArgsType &args) const override {
        auto requestHandler = std::make_shared<RequestHandlerT>(std::move(application), std::move(request));
        requestHandler->start(args);
        return requestHandler;
    }
};


class NET4CXX_COMMON_API UrlSpec: public boost::noncopyable {
public:
    typedef boost::regex RegexType;
    typedef RequestHandler::ArgsType ArgsType;

    UrlSpec(std::string pattern, RequestHandlerFactoryPtr handlerFactory, std::string name);

    UrlSpec(std::string pattern, RequestHandlerFactoryPtr handlerFactory, ArgsType args={}, std::string name={});

    template <typename... Args>
    std::string reverse(Args&&... args) {
        NET4CXX_ASSERT_THROW(!_path.empty(), "Cannot reverse url regex %s", _pattern.c_str());
        NET4CXX_ASSERT_THROW(sizeof...(Args) == _groupCount, "required number of arguments not found");
        return StrUtil::format(_path, UrlParse::quote(boost::lexical_cast<std::string>(args))...);
    }

    const RegexType& getRegex() const {
        return _regex;
    }

    const std::string& getPattern() const {
        return _pattern;
    }

    const std::string& getName() const {
        return _name;
    }

    std::shared_ptr<const BasicRequestHandlerFactory> getHandlerFactory() const {
        return _handlerFactory;
    }

    std::shared_ptr<BasicRequestHandlerFactory> getHandlerFactory() {
        return _handlerFactory;
    }

    const ArgsType& args() const {
        return _args;
    }

    ArgsType& args() {
        return _args;
    }
protected:
    std::tuple<std::string, int> findGroups();

    std::string _pattern;
    RegexType _regex;
    RequestHandlerFactoryPtr _handlerFactory;
    ArgsType _args;
    std::string _name;
    std::string _path;
    int _groupCount;
};

using UrlSpecPtr = std::shared_ptr<UrlSpec>;

NET4CXX_COMMON_API inline std::ostream& operator<<(std::ostream &sout, const UrlSpec &url) {
    sout << StrUtil::format("UrlSpec(%s, %s, name=%s)", url.getPattern(), url.getHandlerFactory()->getName(),
                            url.getName());
    return sout;
}

template <typename HandlerClassT, typename... Args>
UrlSpecPtr url(std::string pattern, Args&&... args) {
    return std::make_shared<UrlSpec>(std::move(pattern),
                                     std::make_shared<RequestHandlerFactory<HandlerClassT>>(),
                                     std::forward<Args>(args)...);
}

using urls = std::vector<UrlSpecPtr>;


class NET4CXX_COMMON_API OutputTransform {
public:
    virtual ~OutputTransform() = default;

    virtual void transformFirstChunk(int &statusCode, HTTPHeaders &headers, std::string &chunk, bool finishing) =0;

    virtual void transformChunk(std::string &chunk, bool finishing) =0;
};


class NET4CXX_COMMON_API GZipContentEncoding: public OutputTransform {
public:
    explicit GZipContentEncoding(const std::shared_ptr<HTTPServerRequest> &request);

    void transformFirstChunk(int &statusCode, HTTPHeaders &headers, std::string &chunk, bool finishing) override;

    void transformChunk(std::string &chunk, bool finishing) override;

    static bool compressibleType(const std::string &ctype);

    static const StringSet CONTENT_TYPES;

    static constexpr int GZIP_LEVEL = 6;
    
    static constexpr int MIN_LENGTH = 1024;
protected:
    bool _gzipping;
    std::shared_ptr<std::stringstream> _gzipValue;
    GzipFile _gzipFile;
};


class NET4CXX_COMMON_API BasicOutputTransformFactory {
public:
    virtual ~BasicOutputTransformFactory() = default;
    virtual OutputTransformPtr create(const std::shared_ptr<HTTPServerRequest> &request) = 0;

};

using OutputTransformFactoryPtr = std::shared_ptr<BasicOutputTransformFactory>;

template<typename OutputTransformT>
class OutputTransformFactory: public BasicOutputTransformFactory {
public:
    OutputTransformPtr create(const std::shared_ptr<HTTPServerRequest> &request) override {
        return std::make_shared<OutputTransformT>(request);
    }
};


class NET4CXX_COMMON_API RequestDispatcher {
public:
    RequestDispatcher(WebAppPtr application, const std::shared_ptr<HTTPConnection> &connection)
            : _application(std::move(application))
            , _connection(connection) {

    }

    void headersReceived(const RequestStartLine &startLine, const std::shared_ptr<HTTPHeaders> &headers) {
        setRequest(std::make_shared<HTTPServerRequest>(_connection.lock(), &startLine, headers));
    }

    void dataReceived(std::string data) {
        if (_handler->hasStreamRequestBody()) {
            _handler->dataReceived(std::move(data));
        } else {
            _chunks.emplace_back(std::move(data));
        }
    }

    void finish() {
        if (!_handler->hasStreamRequestBody()) {
            _request->setBody(boost::join(_chunks, ""));
            _request->parseBody();
        }
        execute();
    }

    void onConnectionClose() {
//        if (_handler->hasStreamRequestBody()) {
//            _handler->onConnectionClose();
//        } else {
//            _chunks.clear();
//        }
        _chunks.clear();
    }

    void execute();
protected:
    void setRequest(std::shared_ptr<HTTPServerRequest> request) {
        _request = std::move(request);
        findHandler();
    }

    void findHandler();

    WebAppPtr _application;
    std::weak_ptr<HTTPConnection> _connection;
    std::shared_ptr<HTTPServerRequest> _request;
    StringVector _chunks;
    std::shared_ptr<RequestHandler> _handler;
    StringVector _pathArgs;
};


class NET4CXX_COMMON_API WebApp: public Factory, public std::enable_shared_from_this<WebApp> {
public:
    typedef std::vector<UrlSpecPtr> HandlersType;
    typedef boost::regex HostPatternType;
    typedef std::pair<HostPatternType, HandlersType> HostHandlerType;
    typedef std::vector<HostHandlerType> HostHandlersType;
    typedef std::map<std::string, UrlSpecPtr> NamedHandlersType;
    typedef std::map<std::string, boost::any> SettingsType;
    typedef std::vector<OutputTransformFactoryPtr> TransformsType;
    typedef std::function<void (std::shared_ptr<const RequestHandler>)> LogFunctionType;

    friend class RequestDispatcher;

    explicit WebApp(HandlersType handlers={}, std::string defaultHost="", TransformsType transforms={},
                    SettingsType settings={});

    virtual ~WebApp() = default;

    ProtocolPtr buildProtocol(const Address &address);

    void addHandlers(std::string hostPattern, HandlersType hostHandlers);

    void addTransform(OutputTransformFactoryPtr transformClass) {
        _transforms.emplace_back(std::move(transformClass));
    }

    template <typename OutputTransformT>
    void addTransform() {
        _transforms.emplace_back(std::make_shared<OutputTransformFactory<OutputTransformT>>());
    }

    std::shared_ptr<RequestDispatcher> startRequest(const std::shared_ptr<HTTPConnection> &connection) {
        return std::make_shared<RequestDispatcher>(shared_from_this(), connection);
    }

    template <typename... Args>
    std::string reverseUrl(const std::string &name, Args&&... args) {
        auto iter = _namedHandlers.find(name);
        if (iter == _namedHandlers.end()) {
            NET4CXX_THROW_EXCEPTION(KeyError, "%s not found in named urls", name);
        }
        return iter->second->reverse(std::forward<Args>(args)...);
    }

    void logRequest(std::shared_ptr<const RequestHandler> handler) const;

    const TransformsType& getTransforms() const {
        return _transforms;
    }

    const std::string& getDefaultHost() const {
        return _defaultHost;
    }

    const SettingsType& getSettings() const {
        return _settings;
    }

    SettingsType& getSettings() {
        return _settings;
    }

    void setNoKeepAlive(bool noKeepAlive) {
        _noKeepAlive = noKeepAlive;
    }

    bool getNoKeepAlive() const {
        return _noKeepAlive;
    }

    void setXHeaders(bool xheaders) {
        _xheaders = xheaders;
    }

    bool getXHeaders() const {
        return _xheaders;
    }

    void setDecompressRequest(bool decompressRequest) {
        _decompressRequest = decompressRequest;
    }

    bool getDecompressRequest() const {
        return _decompressRequest;
    }

    void setChunkSize(size_t chunkSize) {
        _chunkSize = chunkSize;
    }

    size_t getChunkSize() const {
        return _chunkSize;
    }

    void setMaxHeaderSize(size_t maxHeaderSize) {
        _maxHeaderSize = maxHeaderSize;
    }

    size_t getMaxHeaderSize() const {
        return _maxHeaderSize;
    }

    void setMaxBodySize(size_t maxBodySize) {
        _maxBodySize = maxBodySize;
    }

    size_t getMaxBodySize() const {
        return _maxBodySize;
    }

    void setMaxBufferSize(size_t maxBufferSize) {
        _maxBufferSize = maxBufferSize;
    }

    size_t getMaxBufferSize() const {
        return _maxBufferSize;
    }

    void setIdleConnectionTimeout(double idleConnectionTimeout) {
        _idleConnectionTimeout = idleConnectionTimeout;
    }

    double getIdleConnectionTimeout() const {
        return _idleConnectionTimeout;
    }

    void setBodyTimeout(double bodyTimeout) {
        _bodyTimeout = bodyTimeout;
    }

    double getBodyTimeout() const {
        return _bodyTimeout;
    }

    void setProtocol(std::string &&protocol) {
        _protocol = std::move(protocol);
    }

    void setProtocol(const std::string &protocol) {
        _protocol = protocol;
    }

    const std::string& getProtocol() const {
        return _protocol;
    }
protected:
    std::vector<UrlSpecPtr> getHostHandlers(const std::shared_ptr<const HTTPServerRequest> &request);

    TransformsType _transforms;
    HostHandlersType _handlers;
    NamedHandlersType _namedHandlers;
    std::string _defaultHost;
    SettingsType _settings;
    bool _noKeepAlive{false};
    bool _xheaders{false};
    bool _decompressRequest{false};
    size_t _chunkSize{0};
    size_t _maxHeaderSize{0};
    size_t _maxBodySize{0};
    size_t _maxBufferSize{0};
    double _idleConnectionTimeout{3600.0};
    double _bodyTimeout{0.0};
    std::string _protocol;
};


template <typename ReturnT>
std::shared_ptr<ReturnT> makeWebApp(WebApp::HandlersType handlers={}, std::string defaultHost="",
                                    WebApp::TransformsType transforms={}, WebApp::SettingsType settings={}) {
    return std::make_shared<ReturnT>(std::move(handlers), std::move(defaultHost), std::move(transforms),
                                     std::move(settings));
}


template <typename... Args>
std::string RequestHandler::reverseUrl(const std::string &name, Args&&... args) {
    return _application->reverseUrl(name, std::forward<Args>(args)...);
}


NS_END

#define NET4CXX_REMOVE_SLASH()  do { \
    const std::string &path = this->_request->getPath(); \
    if (boost::ends_with(path, "/")) { \
        const std::string &method = this->_request->getMethod(); \
        if (method == "GET" || method == "HEAD") { \
            std::string uri = boost::trim_right_copy_if(path, boost::is_any_of("/")); \
            if (!uri.empty()) { \
                const std::string &query = this->_request->getQuery(); \
                if (!query.empty()) { \
                    uri += "?" + query; \
                } \
                this->redirect(uri, true); \
                return; \
            } \
        } else { \
            NET4CXX_THROW_EXCEPTION(HTTPError) << net4cxx::errinfo_http_code(404); \
        } \
    } \
} while (false)


#define NET4CXX_ADD_SLASH()  do { \
    const std::string &path = this->_request->getPath(); \
    if (!boost::ends_with(path, "/")) { \
        const std::string &method = this->_request->getMethod(); \
        if (method == "GET" || method == "HEAD") { \
            std::string uri = path + "/"; \
            const std::string &query = this->_request->getQuery(); \
            if (!query.empty()) { \
                uri += "?" + query; \
            } \
            this->redirect(uri, true); \
            return; \
        } \
        NET4CXX_THROW_EXCEPTION(HTTPError) << net4cxx::errinfo_http_code(404); \
    } \
} while (false)

#endif //NET4CXX_PLUGINS_WEB_WEB_H
