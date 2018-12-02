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
#include "net4cxx/plugins/web/util.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(BadRequestException, Exception);


class HTTPServerRequest;
class RequestDispatcher;


class NET4CXX_COMMON_API HTTPConnection: public IOStream {
public:
    typedef std::function<void ()> WriteCallbackType;
    typedef std::function<void ()> CloseCallbackType;

    enum State {
        READ_NONE,
        READ_HEADER,
        READ_FIXED_BODY,
        READ_CHUNK_LENGTH,
        READ_CHUNK_DATA,
        READ_CHUNK_ENDS,
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

    void onConnected() override;

    void onDataRead(Byte *data, size_t length) override;

    void onWriteComplete() override;

    void onDisconnected(std::exception_ptr reason) override;

    void writeHeaders(ResponseStartLine startLine, HTTPHeaders &headers, const Byte *chunk = nullptr, size_t length = 0,
                      WriteCallbackType callback = nullptr);

    void writeHeaders(ResponseStartLine startLine, HTTPHeaders &headers, const ByteArray &chunk,
                      WriteCallbackType callback = nullptr) {
        writeHeaders(std::move(startLine), headers, chunk.data(), chunk.size(), std::move(callback));
    }

    void writeHeaders(ResponseStartLine startLine, HTTPHeaders &headers, const char *chunk,
                      WriteCallbackType callback = nullptr) {
        writeHeaders(std::move(startLine), headers, (const Byte *)chunk, strlen(chunk), std::move(callback));
    }

    void writeHeaders(ResponseStartLine startLine, HTTPHeaders &headers, const std::string &chunk,
                      WriteCallbackType callback = nullptr) {
        writeHeaders(std::move(startLine), headers, (const Byte *)chunk.c_str(), chunk.size(), std::move(callback));
    }

    void writeChunk(const Byte *chunk, size_t length, WriteCallbackType callback = nullptr);

    void writeChunk(const ByteArray &chunk, WriteCallbackType callback = nullptr) {
        writeChunk(chunk.data(), chunk.size(), std::move(callback));
    }

    void writeChunk(const char *chunk, WriteCallbackType callback = nullptr) {
        writeChunk((const Byte *)chunk, strlen(chunk), std::move(callback));
    }

    void writeChunk(const std::string &chunk, WriteCallbackType callback = nullptr) {
        writeChunk((const Byte *)chunk.c_str(), chunk.size(), std::move(callback));
    }

    void finish();

    void setCloseCallback(CloseCallbackType callback) {
        _closeCallback = std::move(callback);
    }

    void close(std::exception_ptr reason) override;

    bool getNoKeepAlive() const {
        return _noKeepAlive;
    }

    bool getXHeaders() const {
        return _xheaders;
    }

    const std::string& getRemoteIp() const {
        return _remoteIp;
    }

    const std::string& getProtocol() const {
        return _protocol;
    }
protected:
    void clearCallbacks() {
        _writeCallback = nullptr;
        _closeCallback = nullptr;
    }

    void startRequest();

    std::string formatChunk(const Byte *data, size_t length);

    void readHeaders();

    void readBody();

    void readFixedBody(size_t contentLength) {
        if (contentLength != 0) {
            _bytesRead = 0;
            _bytesToRead = contentLength;
            readFixedBodyBlock();
        } else {
            readFinished();
        }
    }

    void readFixedBodyBlock() {
        _state = READ_FIXED_BODY;
        readBytes(std::min(_chunkSize, _bytesToRead));
    }

    void readChunkLength() {
        _state = READ_CHUNK_LENGTH;
        readUntil("\r\n", 64);
    }

    void readChunkData(size_t chunkLen) {
        _bytesRead = 0;
        _bytesToRead = chunkLen;
        readChunkDataBlock();
    }

    void readChunkDataBlock() {
        _state = READ_CHUNK_DATA;
        readBytes(std::min(_chunkSize, _bytesToRead));
    }

    void readChunkEnds() {
        _state = READ_CHUNK_ENDS;
        readBytes(2);
    }

    void onHeaders(char *data, size_t length);

    void onFixedBody(char *data, size_t length);

    void onChunkLength(char *data, size_t length);

    void onChunkData(char *data, size_t length);

    void onChunkEnds(char *data, size_t length);

    void onHeadersReceived();

    void onDataReceived(char *data, size_t length);

    void readFinished();

    bool canKeepAlive(const RequestStartLine &startLine, const HTTPHeaders &headers);

    void finishRequest();

    void applyXheaders(const HTTPHeaders &headers);

    void unapplyXheaders();

    State _state{READ_NONE};
    bool _noKeepAlive{false};
    bool _xheaders{false};
    bool _decompress{false};
    bool _disconnectOnFinish{false};
    size_t _chunkSize{0};
    size_t _maxHeaderSize{0};
    size_t _maxBodySize{0};
    size_t _totalSize{0};
    size_t _bytesToRead{0};
    size_t _bytesRead{0};
    double _headerTimeout{0.0};
    double _bodyTimeout{0.0};
    std::string _remoteIp;
    std::string _protocol;
    std::string _origRemoteIp;
    std::string _origProtocol;
    DelayedCall _headerTimeoutCall;
    DelayedCall _bodyTimeoutCall;
    std::shared_ptr<HTTPHeaders> _requestHeaders;
    RequestStartLine _requestStartLine;
    ResponseStartLine _responseStartLine;
    std::shared_ptr<RequestDispatcher> _dispatcher;
    bool _chunkingOutput{false};
    bool _pendingWrite{false};
    bool _readFinished{false};
    bool _writeFinished{false};
    WriteCallbackType _writeCallback{nullptr};
    CloseCallbackType _closeCallback{nullptr};
    std::unique_ptr<GzipDecompressor> _decompressor;
    boost::optional<ssize_t> _expectedContentRemaining;
};

using HTTPConnectionPtr = std::shared_ptr<HTTPConnection>;


class NET4CXX_COMMON_API HTTPServerRequest {
public:
    typedef std::function<void ()> WriteCallbackType;
    typedef boost::optional<SimpleCookie> CookiesType;

    HTTPServerRequest(const HTTPServerRequest &) = delete;

    HTTPServerRequest &operator=(const HTTPServerRequest &) = delete;

    HTTPServerRequest(const std::shared_ptr<HTTPConnection> &connection,
                      const RequestStartLine *startLine = nullptr,
                      const std::shared_ptr<HTTPHeaders> &headers = {},
                      const std::string &method = {},
                      const std::string &uri = {},
                      const std::string &version = "HTTP/1.0",
                      std::string body = {},
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

    void write(const Byte *chunk, size_t length, WriteCallbackType callback = nullptr);

    void write(const ByteArray &chunk, WriteCallbackType callback = nullptr) {
        write(chunk.data(), chunk.size(), std::move(callback));
    }

    void write(const char *chunk, WriteCallbackType callback = nullptr) {
        write((const Byte *)chunk, strlen(chunk), std::move(callback));
    }

    void write(const std::string &chunk, WriteCallbackType callback = nullptr) {
        write((const Byte *)chunk.data(), chunk.length(), std::move(callback));
    }

    void finish();

    std::string fullURL() const {
        return _protocol + "://" + _host + _uri;
    }

    double requestTime() const;

    std::shared_ptr<const HTTPHeaders> getHTTPHeaders() const {
        return _headers;
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
        _files[name].emplace_back(std::move(file));
    }

    void parseBody();
protected:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::shared_ptr<HTTPHeaders> _headers;
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
