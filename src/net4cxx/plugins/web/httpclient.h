//
// Created by yuwenyong.vincent on 2018/7/22.
//

#ifndef NET4CXX_PLUGINS_WEB_HTTPCLIENT_H
#define NET4CXX_PLUGINS_WEB_HTTPCLIENT_H

#include "net4cxx/common/common.h"
#include "net4cxx/shared/global/params.h"
#include <boost/optional.hpp>
#include "net4cxx/core/iostream/iostream.h"
#include "net4cxx/plugins/web/httputil.h"


NS_BEGIN


class NET4CXX_COMMON_API HTTPRequest {
protected:
    struct EnableMakeShared;
public:
    typedef std::function<void (ByteArray)> StreamingCallbackType;
    typedef std::function<void (std::string)> HeaderCallbackType;

    explicit HTTPRequest(const EnableMakeShared&) {};

    HTTPHeaders& headers() {
        return _headers;
    }

    const HTTPHeaders& headers() const {
        return _headers;
    }

    void setUrl(std::string url) {
        _url = std::move(url);
    }

    const std::string& getUrl() const {
        return _url;
    }

    void setMethod(std::string method) {
        _method = std::move(method);
    }

    const std::string& getMethod() const {
        return _method;
    }

    void setHeaders(HTTPHeaders headers) {
        _headers = std::move(headers);
    }

    const HTTPHeaders& getHeaders() const {
        return _headers;
    }

    void setBody(std::string body) {
        _body = std::move(body);
    }

    const std::string& getBody() const {
        return _body;
    }

    void setAuthUserName(std::string authUserName) {
        _authUserName = std::move(authUserName);
    }

    const std::string& getAuthUserName() const {
        return _authUserName;
    }

    void setAuthMode(std::string authMode) {
        _authMode = std::move(authMode);
    }

    const std::string& getAuthMode() const {
        return _authMode;
    }

    void setAuthPassword(std::string authPassword) {
        _authPassword = std::move(authPassword);
    }

    const std::string& getAuthPassword() const {
        return _authPassword;
    }

    void setConnectTimeout(double connectTimeout) {
        _connectTimeout = connectTimeout;
    }

    double getConnectTimeout() const {
        return _connectTimeout;
    }

    void setRequestTimeout(double requestTimeout) {
        _requestTimeout = requestTimeout;
    }

    double getRequestTimeout() const {
        return _requestTimeout;
    }

    void setFollowRedirects(bool followRedirects) {
        _followRedirects = followRedirects;
    }

    bool isFollowRedirects() const {
        return _followRedirects;
    }

    void setMaxRedirects(int maxRedirects) {
        _maxRedirects = maxRedirects;
    }

    int getMaxRedirects() const {
        return _maxRedirects;
    }

    void setUserAgent(std::string userAgent) {
        _userAgent = std::move(userAgent);
    }

    const std::string& getUserAgent() const {
        return _userAgent;
    }

    void setUseGzip(bool useGzip) {
        _useGzip = useGzip;
    }

    bool isUseGzip() const {
        return _useGzip;
    }

    void setNetworkInterface(std::string networkInterface) {
        _networkInterface = std::move(networkInterface);
    }

    const std::string& getNetworkInterface() const {
        return _networkInterface;
    }

    void setStreamingCallback(StreamingCallbackType streamingCallback) {
        _streamingCallback = std::move(streamingCallback);
    }

    const StreamingCallbackType& getStreamingCallback() const {
        return _streamingCallback;
    }

    void setHeaderCallback(HeaderCallbackType headerCallback) {
        _headerCallback = std::move(headerCallback);
    }

    const HeaderCallbackType& getHeaderCallback() const {
        return _headerCallback;
    }

    void setProxyHost(std::string proxyHost) {
        _proxyHost = std::move(proxyHost);
    }

    const std::string& getProxyHost() const {
        return _proxyHost;
    }

    void setProxyPort(unsigned short proxyPort) {
        _proxyPort = proxyPort;
    }

    unsigned short getProxyPort() const {
        return _proxyPort;
    }

    void setProxyUserName(std::string proxyUserName) {
        _proxyUserName = std::move(proxyUserName);
    }

    const std::string& getProxyUserName() const {
        return _proxyUserName;
    }

    void setProxyPassword(std::string proxyPassword) {
        _proxyPassword = std::move(proxyPassword);
    }

    const std::string& getProxyPassword() const {
        return _proxyPassword;
    }

    void setAllowNonstandardMethods(bool allowNonstandardMethods) {
        _allowNonstandardMethods = allowNonstandardMethods;
    }

    bool isAllowNonstandardMethods() const {
        return _allowNonstandardMethods;
    }

    void setValidateCert(bool validateCert) {
        _validateCert = validateCert;
    }

    bool isValidateCert() const {
        return _validateCert;
    }

    void setCACerts(std::string caCerts) {
        _caCerts = std::move(caCerts);
    }

    const std::string& getCACerts() const {
        return _caCerts;
    }

    void setClientKey(std::string clientKey) {
        _clientKey = std::move(clientKey);
    }

    const std::string& getClientKey() const {
        return _clientKey;
    }

    void setClientCert(std::string clientCert) {
        _clientCert = std::move(clientCert);
    }

    const std::string& getClientCert() const {
        return _clientCert;
    }

    void setOriginalRequest(std::shared_ptr<HTTPRequest> originalRequest) {
        _originalRequest = std::move(originalRequest);
    }

    std::shared_ptr<HTTPRequest> getOriginalRequest() const {
        return _originalRequest;
    }

    std::shared_ptr<HTTPRequest> clone() const {
        return std::make_shared<HTTPRequest>(*this);
    }

    template <typename ...Args>
    static std::shared_ptr<HTTPRequest> create(Args&& ...args) {
        auto request = std::make_shared<HTTPRequest>(EnableMakeShared{});
        request->init(std::forward<Args>(args)...);
        return request;
    }

protected:
    struct EnableMakeShared {

    };

    BOOST_PARAMETER_MEMBER_FUNCTION((void), init, opts::tag, (
            required
                    (url, (std::string))
    )(
            optional
                    (method, (std::string), "GET")
                    (headers, (HTTPHeaders), HTTPHeaders())
                    (body, (std::string), "")
                    (authUserName, (std::string), "")
                    (authPassword, (std::string), "")
                    (authMode, (std::string), "")
                    (connectTimeout, (double), 20.0)
                    (requestTimeout, (double), 20.0)
                    (ifModifiedSince, (DateTime), DateTime())
                    (followRedirects, (bool), true)
                    (maxRedirects, (int), 5)
                    (userAgent, (std::string), "")
                    (useGzip, (bool), true)
                    (networkInterface, (std::string), "")
                    (streamingCallback, (StreamingCallbackType), nullptr)
                    (headerCallback, (HeaderCallbackType), nullptr)
                    (proxyHost, (std::string), "")
                    (proxyPort, (unsigned short), 0)
                    (proxyUserName, (std::string), "")
                    (proxyPassword, (std::string), "")
                    (allowNonstandardMethods, (bool), false)
                    (validateCert, (bool), true)
                    (caCerts, (std::string), "")
                    (clientKey, (std::string), "")
                    (clientCert, (std::string), "")
    )) {
        _headers = headers;
        if (!ifModifiedSince.is_not_a_date_time()) {
            _headers["If-Modified-Since"] = HTTPUtil::formatTimestamp(ifModifiedSince);
        }

        _proxyHost = proxyHost;
        _proxyPort = proxyPort;
        _proxyUserName = proxyUserName;
        _proxyPassword = proxyPassword;

        _url = url;
        _method = method;
        _body = body;
        _authUserName = authUserName;
        _authPassword = authPassword;
        _authMode = authMode;
        _connectTimeout = connectTimeout;
        _requestTimeout = requestTimeout;
        _followRedirects = followRedirects;
        _maxRedirects = maxRedirects;
        _userAgent = userAgent;
        _useGzip = useGzip;
        _networkInterface = networkInterface;
        _streamingCallback = streamingCallback;
        _headerCallback = headerCallback;
        _allowNonstandardMethods = allowNonstandardMethods;
        _validateCert = validateCert;
        _caCerts = caCerts;
        _clientKey = clientKey;
        _clientCert = clientCert;
        _startTime = TimestampClock::now();
    }

    std::string _url;
    std::string _method;
    HTTPHeaders _headers;
    std::string _body;
    std::string _authUserName;
    std::string _authPassword;
    std::string _authMode;
    double _connectTimeout;
    double _requestTimeout;
    bool _followRedirects;
    int _maxRedirects;
    std::string _userAgent;
    bool _useGzip;
    std::string _networkInterface;
    StreamingCallbackType _streamingCallback;
    HeaderCallbackType _headerCallback;
    std::string _proxyHost;
    unsigned short _proxyPort;
    std::string _proxyUserName;
    std::string _proxyPassword;
    bool _allowNonstandardMethods;
    bool _validateCert;
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
                 std::string body, std::string effectiveUrl, Duration requestTime)
            : _request(std::move(request))
            , _code(code)
            , _headers(std::move(headers))
            , _body(std::move(body))
            , _effectiveUrl(std::move(effectiveUrl))
            , _requestTime(std::move(requestTime)) {
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

    HTTPResponse(HTTPRequestPtr request, int code, std::exception_ptr error, Duration requestTime)
            : _request(std::move(request))
            , _code(code)
            , _error(std::move(error))
            , _requestTime(std::move(requestTime)) {
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
    Duration _requestTime;
};


std::ostream& NET4CXX_COMMON_API operator<<(std::ostream &os, const HTTPResponse &response);


class NET4CXX_COMMON_API HTTPClient: public std::enable_shared_from_this<HTTPClient> {
public:
    explicit HTTPClient(Reactor *reactor=nullptr,
               StringMap hostnameMapping={},
               size_t maxBufferSize=DEFAULT_MAX_BUFFER_SIZE)
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

    template <typename CallbackT, typename ...Args>
    void fetch(const std::string &url, CallbackT &&callback, Args&& ...args) {
        fetch(HTTPRequest::create(url, std::forward<Args>(args)...), std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void fetch(const HTTPRequest &request, CallbackT &&callback) {
        fetch(HTTPRequest::create(request), std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void fetch(HTTPRequestPtr request, CallbackT &&callback);

    const StringMap& getHostnameMapping() const {
        return _hostnameMapping;
    }

    size_t getMaxBufferSize() const {
        return _maxBufferSize;
    }

    template <typename ...Args>
    static std::shared_ptr<HTTPClient> create(Args&& ...args) {
        return std::make_shared<HTTPClient>(std::forward<Args>(args)...);
    }
protected:
    Reactor * _reactor;
    StringMap _hostnameMapping;
    size_t _maxBufferSize;
};

using HTTPClientPtr = std::shared_ptr<HTTPClient>;


class NET4CXX_COMMON_API HTTPClientConnection: public std::enable_shared_from_this<HTTPClientConnection> {
public:
    typedef std::function<void (HTTPResponse)> CallbackType;

    HTTPClientConnection(Reactor *reactor,
                         HTTPClientPtr client,
                         HTTPRequestPtr request,
                         CallbackType callback,
                         size_t maxBufferSize)
            : _reactor(reactor)
            , _client(std::move(client))
            , _request(std::move(request))
            , _callback(std::move(callback))
            , _resolver(_reactor->getIOContext())
            , _maxBufferSize(maxBufferSize) {
        _startTime = TimestampClock::now();
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::HTTPClientConnectionCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~HTTPClientConnection() {
        NET4CXX_Watcher->dec(WatchKeys::HTTPClientConnectionCount);
    }
#endif

    void start();

    template <typename ...Args>
    static std::shared_ptr<HTTPClientConnection> create(Args&& ...args) {
        return std::make_shared<HTTPClientConnection>(std::forward<Args>(args)...);
    }
protected:
    void onResolve(const boost::system::error_code &ec, BaseIOStream::ResolverResultsType addresses);

    BaseIOStreamPtr createStream() const;

    void onTimeout();

    void removeTimeout();

    void onConnect();

    void runCallback(HTTPResponse response);

    void handleException(std::exception_ptr error);

    void onClose();

    void handle1xx(int code);

    void onHeaders(ByteArray data);

    void onBody(ByteArray data);

    void onEndRequest() {
        _stream->close();
    }

    void onChunkLength(ByteArray data);

    void onChunkData(ByteArray data);

    Timestamp _startTime;
    Reactor *_reactor;
    HTTPClientPtr _client;
    HTTPRequestPtr _request;
    CallbackType _callback;
    BaseIOStream::ResolverType _resolver;
    size_t _maxBufferSize;
    boost::optional<int> _code;
    std::unique_ptr<HTTPHeaders> _headers;
    boost::optional<ByteArray> _chunks;
    std::unique_ptr<GzipDecompressor> _decompressor;
    UrlSplitResult _parsed;
    std::string _parsedHostname;
    DelayedCall _timeout;
    BaseIOStreamPtr _stream;
    std::string _reason;

    static const StringSet _SUPPORTED_METHODS;
};


using HTTPClientConnectionPtr = std::shared_ptr<HTTPClientConnection>;


template <typename CallbackT>
void HTTPClient::fetch(HTTPRequestPtr request, CallbackT &&callback) {
    auto connection = HTTPClientConnection::create(_reactor, shared_from_this(), std::move(request),
                                                   std::forward<CallbackT>(callback), _maxBufferSize);
    connection->start();
}

NS_END

#endif //NET4CXX_PLUGINS_WEB_HTTPCLIENT_H
