//
// Created by yuwenyong.vincent on 2018/8/4.
//

#ifndef NET4CXX_PLUGINS_WEB_HTTPSERVER_H
#define NET4CXX_PLUGINS_WEB_HTTPSERVER_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/httputils/cookie.h"
#include "net4cxx/common/httputils/urlparse.h"
#include "net4cxx/core/iostream/tcpserver.h"
#include "net4cxx/plugins/web/httputil.h"


NS_BEGIN


class HTTPServerRequest;

using HTTPServerRequestPtr = std::shared_ptr<HTTPServerRequest>;


class NET4CXX_COMMON_API HTTPServer: public TCPServer {
public:
    typedef std::function<void(HTTPServerRequestPtr)> RequestCallbackType;

    HTTPServer(const HTTPServer &) = delete;

    HTTPServer &operator=(const HTTPServer &) = delete;

    explicit HTTPServer(RequestCallbackType requestCallback,
               bool noKeepAlive = false,
               Reactor *reactor = nullptr,
               bool xheaders = false,
               std::string protocol = "",
               SSLOptionPtr sslOption = nullptr,
               size_t maxBufferSize=0);

    void handleStream(BaseIOStreamPtr stream, std::string address) override;
protected:
    RequestCallbackType _requestCallback;
    bool _noKeepAlive;
    bool _xheaders;
    std::string _protocol;
};


NET4CXX_DECLARE_EXCEPTION(BadRequestException, Exception);


class NET4CXX_COMMON_API HTTPConnection: public std::enable_shared_from_this<HTTPConnection> {
public:
    typedef HTTPServer::RequestCallbackType RequestCallbackType;
    typedef std::function<void ()> WriteCallbackType;
    typedef BaseIOStream::CloseCallbackType CloseCallbackType;
    typedef std::function<void (ByteArray)> HeaderCallbackType;

    class CloseCallbackWrapper {
    public:
        explicit CloseCallbackWrapper(std::shared_ptr<HTTPConnection> connection)
                : _connection(std::move(connection))
                , _needClear(true) {

        }

        CloseCallbackWrapper(CloseCallbackWrapper &&rhs) noexcept
                : _connection(std::move(rhs._connection))
                , _needClear(rhs._needClear) {
            rhs._needClear = false;
        }

        CloseCallbackWrapper& operator=(CloseCallbackWrapper &&rhs) noexcept {
            _connection = std::move(rhs._connection);
            _needClear = rhs._needClear;
            rhs._needClear = false;
            return *this;
        }

        ~CloseCallbackWrapper() {
            if (_needClear) {
                _connection->_closeCallback = nullptr;
            }
        }

        void operator()() {
            _needClear = false;
            _connection->onConnectionClose();
        }
    protected:
        std::shared_ptr<HTTPConnection> _connection;
        bool _needClear;
    };

    class WriteCallbackWrapper {
    public:
        explicit WriteCallbackWrapper(std::shared_ptr<HTTPConnection> connection)
                : _connection(std::move(connection))
                , _needClear(true) {

        }

        WriteCallbackWrapper(WriteCallbackWrapper &&rhs) noexcept
                : _connection(std::move(rhs._connection))
                , _needClear(rhs._needClear) {
            rhs._needClear = false;
        }

        WriteCallbackWrapper& operator=(WriteCallbackWrapper &&rhs) noexcept {
            _connection = std::move(rhs._connection);
            _needClear = rhs._needClear;
            rhs._needClear = false;
            return *this;
        }

        ~WriteCallbackWrapper() {
            if (_needClear) {
                _connection->_writeCallback = nullptr;
            }
        }

        void operator()() {
            _needClear = false;
            _connection->onWriteComplete();
        }
    protected:
        std::shared_ptr<HTTPConnection> _connection;
        bool _needClear;
    };

    HTTPConnection(const HTTPConnection &) = delete;

    HTTPConnection &operator=(const HTTPConnection &) = delete;

    HTTPConnection(BaseIOStreamPtr stream,
                   std::string address,
                   const RequestCallbackType &requestCallback,
                   bool noKeepAlive = false,
                   bool xheaders = false,
                   std::string protocol="")
            : _stream(std::move(stream))
            , _address(std::move(address))
            , _requestCallback(requestCallback)
            , _noKeepAlive(noKeepAlive)
            , _xheaders(xheaders)
            , _protocol(protocol) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::HTTPConnectionCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~HTTPConnection() {
        NET4CXX_Watcher->dec(WatchKeys::HTTPConnectionCount);
    }
#endif

    void setCloseCallback(CloseCallbackType callback) {
        _closeCallback = std::move(callback);
    }

    void close() {
        _stream->close();
        clearRequestState();
    }

    void start();

    void write(const Byte *chunk, size_t length, WriteCallbackType callback= nullptr);

    void finish();

    bool getNoKeepAlive() const {
        return _noKeepAlive;
    }

    bool getXHeaders() const {
        return _xheaders;
    }

    BaseIOStreamPtr getStream() const {
        return _stream;
    }

    HTTPServerRequestPtr getRequest() const {
        return _request;
    }

    template <typename ...Args>
    static std::shared_ptr<HTTPConnection> create(Args&& ...args) {
        return std::make_shared<HTTPConnection>(std::forward<Args>(args)...);
    }
protected:
    void clearRequestState() {
        _request.reset();
        _requestFinished = false;
        _writeCallback = nullptr;
        _closeCallback = nullptr;
    }

    void onConnectionClose();

    void onWriteComplete();

    void finishRequest();

    void onHeaders(ByteArray data);

    void onRequestBody(ByteArray data);

    BaseIOStreamPtr _stream;
    std::string _address;
    const RequestCallbackType &_requestCallback;
    bool _noKeepAlive;
    bool _xheaders;
    std::string _protocol;
    std::shared_ptr<HTTPServerRequest> _request;
    std::weak_ptr<HTTPServerRequest> _requestObserver;
    bool _requestFinished{false};
    WriteCallbackType _writeCallback;
    CloseCallbackType _closeCallback;
//    HeaderCallbackType _headerCallback;
};


using HTTPConnectionPtr = std::shared_ptr<HTTPConnection>;


class NET4CXX_COMMON_API HTTPServerRequest {
public:
    typedef HTTPConnection::WriteCallbackType WriteCallbackType;
    typedef boost::optional<SimpleCookie> CookiesType;

    HTTPServerRequest(const HTTPServerRequest &) = delete;

    HTTPServerRequest &operator=(const HTTPServerRequest &) = delete;

    HTTPServerRequest(HTTPConnectionPtr connection,
                      std::string method,
                      std::string uri,
                      std::string version = "HTTP/1.0",
                      std::unique_ptr<HTTPHeaders> &&headers = nullptr,
                      std::string body = {},
                      std::string remoteIp = {},
                      std::string protocol = {},
                      std::string host = {},
                      HTTPFileListMap files = {});

#ifdef NET4CXX_DEBUG
    ~HTTPServerRequest() {
        NET4CXX_Watcher->dec(WatchKeys::HTTPServerRequestCount);
    }
#endif

    bool supportsHTTP11() const {
        return _version == "HTTP/1.1";
    }

    const SimpleCookie& cookies() const;

    void write(const Byte *chunk, size_t length, WriteCallbackType callback= nullptr);

    void write(const ByteArray &chunk, WriteCallbackType callback= nullptr) {
        write(chunk.data(), chunk.size(), std::move(callback));
    }

    void write(const char *chunk, WriteCallbackType callback= nullptr) {
        write((const Byte *)chunk, strlen(chunk), std::move(callback));
    }

    void write(const std::string &chunk, WriteCallbackType callback= nullptr) {
        write((const Byte *)chunk.data(), chunk.length(), std::move(callback));
    }

    void finish();

    std::string fullURL() const {
        return _protocol + "://" + _host + _uri;
    }

    double requestTime() const;

    const HTTPHeaders* getHTTPHeaders() const {
        return _headers.get();
    }

    const std::string& getMethod() const {
        return _method;
    }

    const std::string& getURI() const {
        return _uri;
    }

    const std::string& getVersion() const {
        return _version;
    }

    void setBody(std::string body) {
        _body = std::move(body);
    }

    const std::string& getBody() const {
        return _body;
    }

    const std::string& getRemoteIp() const {
        return _remoteIp;
    }

    const std::string& getProtocol() const {
        return _protocol;
    }

    const std::string& getHost() const {
        return _host;
    }

    const HTTPFileListMap& getFiles() const {
        return _files;
    }

    HTTPConnectionPtr getConnection() {
        return _connection;
    }

    void setConnection(HTTPConnectionPtr connection) {
        _connection = std::move(connection);
    }

    const std::string& getPath() const {
        return _path;
    }

    const std::string& getQuery() const {
        return _query;
    }

    QueryArgListMap& arguments() {
        return _arguments;
    }

    const QueryArgListMap& getArguments() const {
        return _arguments;
    }

    void addArgument(const std::string &name, std::string value) {
        _arguments[name].emplace_back(std::move(value));
    }

    void addArguments(const std::string &name, StringVector values) {
        for (auto &value: values) {
            addArgument(name, std::move(value));
        }
    }

    QueryArgListMap& queryArguments() {
        return _queryArguments;
    }

    const QueryArgListMap& getQueryArguments() const {
        return _queryArguments;
    }

    QueryArgListMap& bodyArguments() {
        return _bodyArguments;
    }

    const QueryArgListMap& getBodyArguments() const {
        return _bodyArguments;
    }

    HTTPFileListMap& files() {
        return _files;
    }

    void addFile(const std::string &name, HTTPFile file) {
        _files[name].emplace_back(file);
    }

    template <typename ...Args>
    static std::shared_ptr<HTTPServerRequest> create(Args&& ...args) {
        return std::make_shared<HTTPServerRequest>(std::forward<Args>(args)...);
    }
protected:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::unique_ptr<HTTPHeaders> _headers;
    std::string _body;
    std::string _remoteIp;
    std::string _protocol;
    std::string _host;
    HTTPFileListMap _files;
    HTTPConnectionPtr _connection;
    Timestamp _startTime;
    Timestamp _finishTime;
    std::string _path;
    std::string _query;
    QueryArgListMap _arguments;
    QueryArgListMap _queryArguments;
    QueryArgListMap _bodyArguments;
    mutable CookiesType _cookies;
};

std::ostream& NET4CXX_COMMON_API operator<<(std::ostream &os, const HTTPServerRequest &request);

NS_END

#endif //NET4CXX_PLUGINS_WEB_HTTPSERVER_H
