//
// Created by yuwenyong.vincent on 2018/7/22.
//

#ifndef NET4CXX_PLUGINS_WEB_HTTPCLIENT_H
#define NET4CXX_PLUGINS_WEB_HTTPCLIENT_H

#include "net4cxx/common/common.h"
#include <boost/optional.hpp>
#include "net4cxx/core/protocols/iostream.h"
#include "net4cxx/plugins/web/httputil.h"


NS_BEGIN


class NET4CXX_COMMON_API HTTPRequest: public std::enable_shared_from_this<HTTPRequest> {
public:
    typedef std::function<void (ByteArray)> StreamingCallbackType;
    typedef std::function<void (std::string)> HeaderCallbackType;

    explicit HTTPRequest(std::string url,
                         HTTPHeaders headers = {},
                         const DateTime &ifModifiedSince = {})
            : HTTPRequest(std::move(url), "GET", std::move(headers), ifModifiedSince) {

    }

    explicit HTTPRequest(std::string url,
                         std::string method,
                         HTTPHeaders headers = {},
                         const DateTime &ifModifiedSince = {})
            : _url(std::move(url))
            , _method(std::move(method))
            , _headers{std::move(headers)} {
        if (!ifModifiedSince.is_not_a_date_time()) {
            _headers["If-Modified-Since"] = HTTPUtil::formatTimestamp(ifModifiedSince);
        }
        _startTime = TimestampClock::now();
    }

    HTTPHeaders& headers() {
        return _headers;
    }

    const HTTPHeaders& headers() const {
        return _headers;
    }

    std::shared_ptr<HTTPRequest> setUrl(std::string &&url) {
        _url = std::move(url);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setUrl(const std::string &url) {
        _url = url;
        return shared_from_this();
    }

    const std::string& getUrl() const {
        return _url;
    }

    std::shared_ptr<HTTPRequest> setMethod(std::string &&method) {
        _method = std::move(method);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setMethod(const std::string &method) {
        _method = method;
        return shared_from_this();
    }

    const std::string& getMethod() const {
        return _method;
    }

    std::shared_ptr<HTTPRequest> setHeaders(HTTPHeaders &&headers) {
        _headers = std::move(headers);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setHeaders(const HTTPHeaders &headers) {
        _headers = headers;
        return shared_from_this();
    }

    const HTTPHeaders& getHeaders() const {
        return _headers;
    }

    std::shared_ptr<HTTPRequest> setBody(std::string &&body) {
        _body = std::move(body);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setBody(const std::string &body) {
        _body = body;
        return shared_from_this();
    }

    const std::string& getBody() const {
        return _body;
    }

    std::shared_ptr<HTTPRequest> setAuthUserName(std::string &&authUserName) {
        _authUserName = std::move(authUserName);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setAuthUserName(const std::string &authUserName) {
        _authUserName = authUserName;
        return shared_from_this();
    }

    const std::string& getAuthUserName() const {
        return _authUserName;
    }

    std::shared_ptr<HTTPRequest> setAuthMode(std::string &&authMode) {
        _authMode = std::move(authMode);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setAuthMode(const std::string &authMode) {
        _authMode = authMode;
        return shared_from_this();
    }

    const std::string& getAuthMode() const {
        return _authMode;
    }

    std::shared_ptr<HTTPRequest> setAuthPassword(std::string &&authPassword) {
        _authPassword = std::move(authPassword);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setAuthPassword(const std::string &authPassword) {
        _authPassword = authPassword;
        return shared_from_this();
    }

    const std::string& getAuthPassword() const {
        return _authPassword;
    }

    std::shared_ptr<HTTPRequest> setConnectTimeout(double connectTimeout) {
        _connectTimeout = connectTimeout;
        return shared_from_this();
    }

    double getConnectTimeout() const {
        return _connectTimeout;
    }

    std::shared_ptr<HTTPRequest> setRequestTimeout(double requestTimeout) {
        _requestTimeout = requestTimeout;
        return shared_from_this();
    }

    double getRequestTimeout() const {
        return _requestTimeout;
    }

    std::shared_ptr<HTTPRequest> setFollowRedirects(bool followRedirects) {
        _followRedirects = followRedirects;
        return shared_from_this();
    }

    bool isFollowRedirects() const {
        return _followRedirects;
    }

    std::shared_ptr<HTTPRequest> setMaxRedirects(int maxRedirects) {
        _maxRedirects = maxRedirects;
        return shared_from_this();
    }

    int getMaxRedirects() const {
        return _maxRedirects;
    }

    std::shared_ptr<HTTPRequest> setUserAgent(std::string &&userAgent) {
        _userAgent = std::move(userAgent);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setUserAgent(const std::string &userAgent) {
        _userAgent = userAgent;
        return shared_from_this();
    }

    const std::string& getUserAgent() const {
        return _userAgent;
    }

    std::shared_ptr<HTTPRequest> setUseGzip(bool useGzip) {
        _useGzip = useGzip;
        return shared_from_this();
    }

    bool isUseGzip() const {
        return _useGzip;
    }

    std::shared_ptr<HTTPRequest> setNetworkInterface(std::string &&networkInterface) {
        _networkInterface = std::move(networkInterface);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setNetworkInterface(const std::string &networkInterface) {
        _networkInterface = networkInterface;
        return shared_from_this();
    }

    const std::string& getNetworkInterface() const {
        return _networkInterface;
    }

    std::shared_ptr<HTTPRequest> setStreamingCallback(StreamingCallbackType &&streamingCallback) {
        _streamingCallback = std::move(streamingCallback);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setStreamingCallback(const StreamingCallbackType &streamingCallback) {
        _streamingCallback = streamingCallback;
        return shared_from_this();
    }

    const StreamingCallbackType& getStreamingCallback() const {
        return _streamingCallback;
    }

    std::shared_ptr<HTTPRequest> setHeaderCallback(HeaderCallbackType &&headerCallback) {
        _headerCallback = std::move(headerCallback);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setHeaderCallback(const HeaderCallbackType &headerCallback) {
        _headerCallback = headerCallback;
        return shared_from_this();
    }

    const HeaderCallbackType& getHeaderCallback() const {
        return _headerCallback;
    }

    std::shared_ptr<HTTPRequest> setProxyHost(std::string &&proxyHost) {
        _proxyHost = std::move(proxyHost);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setProxyHost(const std::string &proxyHost) {
        _proxyHost = proxyHost;
        return shared_from_this();
    }

    const std::string& getProxyHost() const {
        return _proxyHost;
    }

    std::shared_ptr<HTTPRequest> setProxyPort(unsigned short proxyPort) {
        _proxyPort = proxyPort;
        return shared_from_this();
    }

    unsigned short getProxyPort() const {
        return _proxyPort;
    }

    std::shared_ptr<HTTPRequest> setProxyUserName(std::string &&proxyUserName) {
        _proxyUserName = std::move(proxyUserName);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setProxyUserName(const std::string &proxyUserName) {
        _proxyUserName = proxyUserName;
        return shared_from_this();
    }

    const std::string& getProxyUserName() const {
        return _proxyUserName;
    }

    std::shared_ptr<HTTPRequest> setProxyPassword(std::string &&proxyPassword) {
        _proxyPassword = std::move(proxyPassword);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setProxyPassword(const std::string &proxyPassword) {
        _proxyPassword = proxyPassword;
        return shared_from_this();
    }

    const std::string& getProxyPassword() const {
        return _proxyPassword;
    }

    std::shared_ptr<HTTPRequest> setAllowNonstandardMethods(bool allowNonstandardMethods) {
        _allowNonstandardMethods = allowNonstandardMethods;
        return shared_from_this();
    }

    bool isAllowNonstandardMethods() const {
        return _allowNonstandardMethods;
    }

    std::shared_ptr<HTTPRequest> setValidateCert(bool validateCert) {
        _validateCert = validateCert;
        return shared_from_this();
    }

    bool isValidateCert() const {
        return _validateCert;
    }

    std::shared_ptr<HTTPRequest> setCACerts(std::string &&caCerts) {
        _caCerts = std::move(caCerts);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setCACerts(const std::string &caCerts) {
        _caCerts = caCerts;
        return shared_from_this();
    }

    const std::string& getCACerts() const {
        return _caCerts;
    }

    std::shared_ptr<HTTPRequest> setClientKey(std::string &&clientKey) {
        _clientKey = std::move(clientKey);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setClientKey(const std::string &clientKey) {
        _clientKey = clientKey;
        return shared_from_this();
    }

    const std::string& getClientKey() const {
        return _clientKey;
    }

    std::shared_ptr<HTTPRequest> setClientCert(std::string &&clientCert) {
        _clientCert = std::move(clientCert);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setClientCert(const std::string &clientCert) {
        _clientCert = clientCert;
        return shared_from_this();
    }

    const std::string& getClientCert() const {
        return _clientCert;
    }

    std::shared_ptr<HTTPRequest> setOriginalRequest(std::shared_ptr<HTTPRequest> &&originalRequest) {
        _originalRequest = std::move(originalRequest);
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> setOriginalRequest(const std::shared_ptr<HTTPRequest> &originalRequest) {
        _originalRequest = originalRequest;
        return shared_from_this();
    }

    std::shared_ptr<HTTPRequest> getOriginalRequest() const {
        return _originalRequest;
    }

    Timestamp getStartTime() const {
        return _startTime;
    }

    std::shared_ptr<HTTPRequest> clone() const {
        return std::make_shared<HTTPRequest>(*this);
    }

    template <typename ...Args>
    static std::shared_ptr<HTTPRequest> create(Args&& ...args) {
        auto request = std::make_shared<HTTPRequest>(std::forward<Args>(args)...);
        return request;
    }

protected:
    std::string _url;
    std::string _method{"GET"};
    HTTPHeaders _headers;
    std::string _body;
    std::string _authUserName;
    std::string _authPassword;
    std::string _authMode;
    double _connectTimeout{20.0};
    double _requestTimeout{20.0};
    bool _followRedirects{true};
    int _maxRedirects{5};
    std::string _userAgent;
    bool _useGzip{true};
    std::string _networkInterface;
    StreamingCallbackType _streamingCallback;
    HeaderCallbackType _headerCallback;
    std::string _proxyHost;
    unsigned short _proxyPort{0};
    std::string _proxyUserName;
    std::string _proxyPassword;
    bool _allowNonstandardMethods{false};
    bool _validateCert{true};
    std::string _caCerts;
    std::string _clientKey;
    std::string _clientCert;
    Timestamp _startTime;
    std::shared_ptr<HTTPRequest> _originalRequest;
};


using HTTPRequestPtr = std::shared_ptr<HTTPRequest>;


class NET4CXX_COMMON_API HTTPResponse {
public:
    HTTPResponse() = default;

    HTTPResponse(HTTPRequestPtr request, int code, std::string reason, HTTPHeaders headers,
                 std::string body, std::string effectiveUrl, const Duration &requestTime)
            : _request(std::move(request))
            , _code(code)
            , _headers(std::move(headers))
            , _body(std::move(body))
            , _effectiveUrl(std::move(effectiveUrl))
            , _requestTime(requestTime) {
        if (reason.empty()) {
            _reason = HTTPUtil::getHTTPReason(code);
        } else {
            _reason = std::move(reason);
        }
        if (!_error) {
            if (_code < 200 || _code >= 300) {
                _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(HTTPError, "") << errinfo_http_code(code));
            }
        }
    }

    HTTPResponse(HTTPRequestPtr request, int code, std::exception_ptr error, const Duration &requestTime)
            : _request(std::move(request))
            , _code(code)
            , _error(error)
            , _requestTime(requestTime) {
        _reason = HTTPUtil::getHTTPReason(code);
        if (!_error) {
            if (_code < 200 || _code >= 300) {
                _error = std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(HTTPError, "") << errinfo_http_code(code));
            }
        }
    }

    HTTPRequestPtr getRequest() const {
        return _request;
    }

    int getCode() const {
        return _code;
    }

    const std::string& getReason() const {
        return _reason;
    }

    const HTTPHeaders& getHeaders() const {
        return _headers;
    }

    const std::string& getBody() const {
        return _body;
    }

    const std::string& getEffectiveUrl() const {
        return _effectiveUrl;
    }

    std::exception_ptr getError() const {
        return _error;
    }

    const Duration& getRequestTime() const {
        return _requestTime;
    }

    void rethrow() const {
        if (_error) {
            std::rethrow_exception(_error);
        }
    }
protected:
    HTTPRequestPtr _request;
    int _code{200};
    std::string _reason;
    HTTPHeaders _headers;
    std::string _body;
    std::string _effectiveUrl;
    std::exception_ptr _error;
    Duration _requestTime{};
};


NET4CXX_COMMON_API std::ostream& operator<<(std::ostream &os, const HTTPResponse &response);


class NET4CXX_COMMON_API HTTPClient: public std::enable_shared_from_this<HTTPClient> {
public:
    typedef std::function<void (const HTTPResponse &)> CallbackType;

    explicit HTTPClient(Reactor *reactor=nullptr,
               StringMap hostnameMapping={},
               size_t maxBufferSize=IOStream::DEFAULT_MAX_BUFFER_SIZE)
            : _reactor(reactor ? reactor : Reactor::current())
            , _hostnameMapping(std::move(hostnameMapping))
            , _maxBufferSize(maxBufferSize) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::HTTPClientCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~HTTPClient() {
        NET4CXX_Watcher->dec(WatchKeys::HTTPClientCount);
    }
#endif

    void close() {

    }

    DeferredPtr fetch(const std::string &url, CallbackType callback= nullptr) {
        return fetch(HTTPRequest::create(url), std::move(callback));
    }

    DeferredPtr fetch(const HTTPRequest &request, CallbackType callback= nullptr) {
        return fetch(HTTPRequest::create(request), std::move(callback));
    }

    DeferredPtr fetch(HTTPRequestPtr request, CallbackType callback= nullptr);

    const StringMap& getHostnameMapping() const {
        return _hostnameMapping;
    }

    size_t getMaxBufferSize() const {
        return _maxBufferSize;
    }

    Reactor* reactor() {
        return _reactor;
    }

    template <typename ...Args>
    static std::shared_ptr<HTTPClient> create(Args&& ...args) {
        return std::make_shared<HTTPClient>(std::forward<Args>(args)...);
    }
protected:
    void fetchImpl(HTTPRequestPtr request, CallbackType &&callback);

    Reactor * _reactor;
    StringMap _hostnameMapping;
    size_t _maxBufferSize;
};

using HTTPClientPtr = std::shared_ptr<HTTPClient>;


class NET4CXX_COMMON_API HTTPClientConnection: public IOStream {
public:
    typedef std::function<void (const HTTPResponse &)> CallbackType;

    enum State {
        INITIAL,
        READ_HEADER,
        READ_BODY,
        READ_CHUNK_LENGTH,
        READ_CHUNK_DATA,
    };

    explicit HTTPClientConnection(HTTPClientPtr client,
                                  HTTPRequestPtr request,
                                  CallbackType callback,
                                  size_t maxBufferSize = 0)
            : IOStream(maxBufferSize)
            , _client(std::move(client))
            , _request(std::move(request))
            , _callback(std::move(callback)) {
        _startTime = TimestampClock::now();
#ifdef NET4CXX_DEBUG
            NET4CXX_Watcher->inc(WatchKeys::HTTPClientConnectionCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~HTTPClientConnection() override {
        NET4CXX_Watcher->dec(WatchKeys::HTTPClientConnectionCount);
    }
#endif

    void startProcessing();

    void connectionMade() override;

    void dataRead(Byte *data, size_t length) override;

    void connectionClose(std::exception_ptr reason) override;
protected:
    void startConnecting(const std::string &host, unsigned short port);

    void onTimeout();

    void removeTimeout();

    void runCallback(HTTPResponse response);

    void handleException(std::exception_ptr error);

    void handle1xx(int code);

    void onHeaders(Byte *data, size_t length);

    void onBody(Byte *data, size_t length);

    void onEndRequest() {
        loseConnection();
    }

    void onChunkLength(Byte *data, size_t length);

    void onChunkData(Byte *data, size_t length);

    State _state{INITIAL};
    Timestamp _startTime;
    HTTPClientPtr _client;
    HTTPRequestPtr _request;
    CallbackType _callback;
    boost::optional<int> _code;
    std::unique_ptr<HTTPHeaders> _headers;
    boost::optional<ByteArray> _chunks;
    std::unique_ptr<GzipDecompressor> _decompressor;
    UrlSplitResult _parsed;
    std::string _parsedHostname;
    DelayedCall _timeout;
    std::string _reason;

    static const StringSet _SUPPORTED_METHODS;
};

using HTTPClientConnectionPtr = std::shared_ptr<HTTPClientConnection>;


NS_END

#endif //NET4CXX_PLUGINS_WEB_HTTPCLIENT_H
