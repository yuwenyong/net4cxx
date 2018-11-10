//
// Created by yuwenyong.vincent on 2018/8/4.
//

#ifndef NET4CXX_PLUGINS_WEB_HTTPSERVER_H
#define NET4CXX_PLUGINS_WEB_HTTPSERVER_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/httputils/cookie.h"
#include "net4cxx/common/httputils/urlparse.h"
#include "net4cxx/core/protocols/iostream.h"
#include "net4cxx/plugins/web/httputil.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(BadRequestException, Exception);


class HTTPServerRequest;


class NET4CXX_COMMON_API HTTPConnection: public IOStream {
public:
    typedef std::function<void ()> CloseCallbackType;

    enum State {
        INITIAL,
        READ_HEADER,
        READ_BODY,
        PROCESS_REQUEST,
    };

    explicit HTTPConnection(size_t maxBufferSize=0): IOStream(maxBufferSize) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::HTTPConnectionCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~HTTPConnection() override {
        NET4CXX_Watcher->dec(WatchKeys::HTTPConnectionCount);
    }
#endif

    void connectionMade() override;

    void dataRead(Byte *data, size_t length) override;

    void connectionClose(std::exception_ptr reason) override;

    void writeChunk(const Byte *chunk, size_t length);

    void writeChunk(const ByteArray &chunk) {
        writeChunk(chunk.data(), chunk.size());
    }

    void writeChunk(const char *chunk) {
        writeChunk((const Byte *)chunk, strlen(chunk));
    }

    void writeChunk(const std::string &chunk) {
        writeChunk((const Byte *)chunk.c_str(), chunk.size());
    }

    void finish();

    void setCloseCallback(CloseCallbackType callback) {
        _closeCallback = std::move(callback);
    }

    void close() {
        closeStream();
        clearRequestState();
    }

    bool getNoKeepAlive() const {
        return _noKeepAlive;
    }

    bool getXHeaders() const {
        return _xheaders;
    }

    std::shared_ptr<const HTTPServerRequest> getRequest() const {
        return _request;
    }

    std::shared_ptr<HTTPServerRequest> getRequest() {
        return _request;
    }
protected:
    void clearRequestState() {
        _request.reset();
        _requestFinished = false;
        _closeCallback = nullptr;
    }

    void onHeaders(Byte *data, size_t length);

    void onRequestBody(Byte *data, size_t length);

    void onProcessRequest();

    void onWriteComplete();

    void finishRequest();

    State _state{INITIAL};
    std::string _address;
    bool _noKeepAlive{false};
    bool _xheaders{false};
    std::string _protocol;
    std::shared_ptr<HTTPServerRequest> _request;
    bool _requestFinished{false};
    CloseCallbackType _closeCallback{nullptr};
};

using HTTPConnectionPtr = std::shared_ptr<HTTPConnection>;


class NET4CXX_COMMON_API HTTPServerRequest {
public:
    typedef boost::optional<SimpleCookie> CookiesType;

    HTTPServerRequest(const HTTPServerRequest &) = delete;

    HTTPServerRequest &operator=(const HTTPServerRequest &) = delete;

    HTTPServerRequest(std::shared_ptr<HTTPConnection> connection,
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

    void write(const Byte *chunk, size_t length);

    void write(const ByteArray &chunk) {
        write(chunk.data(), chunk.size());
    }

    void write(const char *chunk) {
        write((const Byte *)chunk, strlen(chunk));
    }

    void write(const std::string &chunk) {
        write((const Byte *)chunk.data(), chunk.length());
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

    std::shared_ptr<const HTTPConnection> getConnection() const {
        return _connection.lock();
    }

    std::shared_ptr<HTTPConnection> getConnection() {
        return _connection.lock();
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
    std::weak_ptr<HTTPConnection> _connection;
    Timestamp _startTime;
    Timestamp _finishTime;
    std::string _path;
    std::string _query;
    QueryArgListMap _arguments;
    QueryArgListMap _queryArguments;
    QueryArgListMap _bodyArguments;
    mutable CookiesType _cookies;
};

using HTTPServerRequestPtr = std::shared_ptr<HTTPServerRequest>;

NET4CXX_COMMON_API std::ostream& operator<<(std::ostream &os, const HTTPServerRequest &request);

NS_END

#endif //NET4CXX_PLUGINS_WEB_HTTPSERVER_H
