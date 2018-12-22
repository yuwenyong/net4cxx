//
// Created by yuwenyong.vincent on 2018/7/22.
//

#include "net4cxx/plugins/web/httpclient.h"
#include "net4cxx/common/crypto/base64.h"
#include "net4cxx/core/network/defer.h"
#include "net4cxx/core/network/endpoints.h"


NS_BEGIN


void HTTPRequestBodyProducer::write(const Byte *chunk, size_t length) {
    auto connection = _connection.lock();
    if (connection) {
        connection->writeChunk(chunk, length);
    } else {
        NET4CXX_THROW_EXCEPTION(RuntimeError, "connection is not active");
    }
}

void HTTPRequestBodyProducer::finish() {
    auto connection = _connection.lock();
    if (connection) {
        connection->writeFinished();
    } else {
        NET4CXX_THROW_EXCEPTION(RuntimeError, "connection is not active");
    }
}


std::ostream &operator<<(std::ostream &os, const HTTPResponse &response) {
    StringVector args;
    args.emplace_back(StrUtil::format("code=%d", response.getCode()));
    args.emplace_back(StrUtil::format("reason=\"%s\"", response.getReason()));
    args.emplace_back(StrUtil::format("headers=\"%s\"", response.getHeaders()));
    if (!response.getBody().empty()) {
        args.emplace_back(StrUtil::format("body=\"%s\"", response.getBody()));
    }
    args.emplace_back(StrUtil::format("effectiveUrl=\"%s\"", response.getEffectiveUrl()));
    try {
        response.rethrow();
    } catch (std::exception &e) {
        args.emplace_back(StrUtil::format("error=\"%s\"", e.what()));
    } catch (...) {
        args.emplace_back(StrUtil::format("error=\"Unknown exception\""));
    }
    auto elapse = std::chrono::duration_cast<std::chrono::microseconds>(response.getRequestTime());
    double requestTime = elapse.count() / 1000000 + elapse.count() % 1000000 / 1000000.0;
    args.emplace_back(StrUtil::format("requestTime=\"%0.6fs\"", requestTime));
    os << "HTTPResponse(" << boost::join(args, ",") << ")";
    return os;
}


DeferredPtr HTTPClient::fetch(HTTPRequestPtr request, CallbackType callback, bool raiseError) {
    if (_closed) {
        NET4CXX_THROW_EXCEPTION(RuntimeError, "client already closed");
    }
    DeferredPtr result = makeDeferred();
    if (callback) {
        result->addBoth([request, callback = std::move(callback)](DeferredValue value) {
            if (value.isError()) {
                HTTPResponse response(request, 599, value.asError(), TimestampClock::now() - request->getStartTime());
                callback(response);
            } else {
                callback(*value.asValue<HTTPResponse>());
            }
            return value;
        });
    }
    fetchImpl(std::move(request), [result, raiseError](HTTPResponse response) {
        if (raiseError && response.getError()) {
            result->errback(response.getError());
        } else {
            result->callback(std::move(response));
        }
    });
    return result;
}

void HTTPClient::fetchImpl(HTTPRequestPtr request, CallbackType &&callback) {
    auto connection = std::make_shared<HTTPClientConnection>(shared_from_this(), std::move(request),
                                                             std::move(callback), _maxBufferSize, _maxHeaderSize);
    connection->startRequest();
}


void HTTPClientConnection::startRequest() {
    try {
        _parsed = UrlParse::urlSplit(_request->getUrl());
        const std::string &scheme = _parsed.getScheme();
        if (scheme != "http" && scheme != "https") {
            NET4CXX_THROW_EXCEPTION(ValueError, "Unsupported url scheme: %s", _request->getUrl());
        }
        std::string netloc = _parsed.getNetloc();
        if (netloc.find('@') != std::string::npos) {
            std::string userpass;
            std::tie(userpass, std::ignore, netloc) = StrUtil::rpartition(netloc, "@");
        }
        std::string host;
        boost::optional<unsigned short> port;
        std::tie(host, port) = HTTPUtil::splitHostAndPort(netloc);
        if (!port) {
            port = (unsigned short) (scheme == "https" ? 443 : 80);
        }
        const boost::regex ipv6(R"(^\[.*\]$)");
        if (boost::regex_match(host, ipv6)) {
            host = host.substr(1, host.size() - 2);
        }
        _parsedHostname = host;
        const StringMap &hostnameMapping = _client->getHostnameMapping();
        auto iter = hostnameMapping.find(host);
        if (iter != hostnameMapping.end()) {
            host = iter->second;
        }
        startConnecting(host, *port);
    } catch (...) {
        handleException(std::current_exception());
    }
}

void HTTPClientConnection::onConnected() {
    try {
        if (!_callback) {
            close(nullptr);
            return;
        }
        double requestTimeout = _request->getRequestTimeout();
        if (requestTimeout != 0.0) {
            _timeout = _client->reactor()->callLater(requestTimeout, [this, self = shared_from_this()]() {
                try {
                    onTimeout("during request");
                } catch (...) {
                    handleException(std::current_exception());
                }
            });
        }
        const std::string &method = _request->getMethod();
        if (_SUPPORTED_METHODS.find(method) == _SUPPORTED_METHODS.end() && !_request->isAllowNonstandardMethods()) {
            NET4CXX_THROW_EXCEPTION(KeyError, "unknown method %s", method);
        }
        if (!_request->getNetworkInterface().empty()) {
            NET4CXX_THROW_EXCEPTION(NotImplementedError, "NetworkInterface not supported");
        }
        if (!_request->getProxyHost().empty()) {
            NET4CXX_THROW_EXCEPTION(NotImplementedError, "ProxyHost not supported");
        }
        if (_request->getProxyPort() != 0) {
            NET4CXX_THROW_EXCEPTION(NotImplementedError, "ProxyPort not supported");
        }
        if (!_request->getProxyUserName().empty()) {
            NET4CXX_THROW_EXCEPTION(NotImplementedError, "ProxyUser not supported");
        }
        if (!_request->getProxyPassword().empty()) {
            NET4CXX_THROW_EXCEPTION(NotImplementedError, "ProxyPassword not supported");
        }
        if (!_request->getProxyAuthMode().empty()) {
            NET4CXX_THROW_EXCEPTION(NotImplementedError, "ProxyAuthMode not supported");
        }
        auto &headers = _request->headers();
        if (!headers.has("Connection")) {
            headers["Connection"] = "close";
        }
        if (!headers.has("Host")) {
            if (_parsed.getNetloc().find('@') != std::string::npos) {
                std::string host;
                std::tie(std::ignore, std::ignore, host) = StrUtil::rpartition(_parsed.getNetloc(), "@");
                headers["Host"] = host;
            } else {
                headers["Host"] = _parsed.getNetloc();
            }
        }
        std::string userName, password;
        if (_parsed.getUserName()) {
            userName = *_parsed.getUserName();
            password = *_parsed.getPassword();
        } else if (!_request->getAuthUserName().empty()) {
            userName = _request->getAuthUserName();
            password = _request->getAuthPassword();
        }
        if (!userName.empty()) {
            const std::string &authMode = _request->getAuthMode();
            if (!authMode.empty() && authMode != "basic") {
                NET4CXX_THROW_EXCEPTION(ValueError, "unsupported auth mode %s", authMode);
            }
            std::string auth = userName + ":" + password;
            auth = "Basic " + Base64::b64encode(auth);
            headers["Authorization"] = auth;
        }
        const std::string &userAgent = _request->getUserAgent();
        if (!userAgent.empty()) {
            headers["User-Agent"] = userAgent;
        }
        const std::string &requestBody = _request->getBody();
        if (!_request->isAllowNonstandardMethods()) {
            bool bodyExpected = method == "POST" || method == "PATCH" || method == "PUT";
            bool bodyPresent = !requestBody.empty() || _request->getBodyProducer();
            if (bodyExpected != bodyPresent) {
                NET4CXX_THROW_EXCEPTION(ValueError, "Body must %sbe empty for method %s", bodyExpected ? "not " : "",
                                        method);
            }
        }
        if (_request->getExpect100Continue()) {
            headers["Expect"] = "100-continue";
        }
        if (!requestBody.empty()) {
            headers["Content-Length"] = std::to_string(requestBody.size());
        }
        if (method == "POST" && !headers.has("Content-Type")) {
            headers["Content-Type"] = "application/x-www-form-urlencoded";
        }
        if (_request->getDecompressResponse()) {
            headers["Accept-Encoding"] = "gzip";
        }
        const std::string &parsedPath = _parsed.getPath();
        const std::string &parsedQuery = _parsed.getQuery();
        std::string reqPath = parsedPath.empty() ? "/" : parsedPath;
        if (!parsedQuery.empty()) {
            reqPath += '?';
            reqPath += parsedQuery;
        }
        setNoDelay(true);
        auto startLine = RequestStartLine(method, reqPath, "");
        writeHeaders(startLine, _request->headers());
        if (_request->getExpect100Continue()) {
            readResponse();
        } else {
            writeBody(true);
        }
    } catch (...) {
        handleException(std::current_exception());
    }
}

void HTTPClientConnection::onWriteComplete() {
    if (_writeFinished) {
        setNoDelay(false);
    }
}

void HTTPClientConnection::onDataRead(Byte *data, size_t length) {
    try {
        switch (_state) {
            case READ_HEADER: {
                onHeaders((char *) data, length);
                break;
            }
            case READ_FIXED_BODY: {
                onFixedBody((char *) data, length);
                break;
            }
            case READ_CHUNK_LENGTH: {
                onChunkLength((char *) data, length);
                break;
            }
            case READ_CHUNK_DATA: {
                onChunkData((char *) data, length);
                break;
            }
            case READ_CHUNK_ENDS: {
                onChunkEnds((char *) data, length);
                break;
            }
            case READ_LAST_CHUNK_ENDS: {
                onLastChunkEnds((char *) data, length);
                break;
            }
            case READ_UNTIL_CLOSE: {
                onReadUntilClose((char *) data, length);
                break;
            }
            default: {
                NET4CXX_ASSERT_MSG(false, "Unreachable");
                break;
            }
        }
    } catch (...) {
        handleException(std::current_exception());
    }
}

void HTTPClientConnection::onDisconnected(std::exception_ptr reason) {
    try {
        if (_callback) {
            std::string message("Connection closed");
            if (reason) {
                try {
                    std::rethrow_exception(reason);
                } catch (std::exception &e) {
                    message = e.what();
                }
            }
            NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(599) << errinfo_http_reason(message);
        }
    } catch (...) {
        handleException(std::current_exception());
    }
}

void HTTPClientConnection::writeFinished() {
    if (_expectedContentRemaining && *_expectedContentRemaining != 0) {
        try {
            NET4CXX_THROW_EXCEPTION(HTTPOutputError, "Tried to write %d bytes less than Content-Length",
                                    *_expectedContentRemaining);
        } catch (...) {
            close(std::current_exception());
            throw;
        }
    }
    if (_chunkingOutput) {
        write("0\r\n\r\n", true);
    }
    _writeFinished = true;
    if (!_writeCallback) {
        setNoDelay(false);
    }
}

const StringSet HTTPClientConnection::_SUPPORTED_METHODS = {"GET", "HEAD", "POST", "PUT", "DELETE", "PATCH", "OPTIONS"};

void HTTPClientConnection::startConnecting(const std::string &host, unsigned short port) {
    DeferredPtr connectDeferred;
    if (_parsed.getScheme() == "https") {
        SSLOptionPtr sslOption;
        if (_request->getSSLOption()) {
            sslOption = _request->getSSLOption();
        } else {
            SSLClientOptionBuilder builder;
            if (_request->isValidateCert()) {
                builder.setVerifyMode(SSLVerifyMode::CERT_REQUIRED);
                builder.setCheckHost(_parsedHostname);
            } else {
                builder.setVerifyMode(SSLVerifyMode::CERT_NONE);
            }
            const std::string &caCerts = _request->getCACerts();
            if (!caCerts.empty()) {
                builder.setVerifyFile(caCerts);
            }
            const std::string &clientKey = _request->getClientKey();
            if (!clientKey.empty()) {
                builder.setKeyFile(clientKey);
            }
            const std::string &clientCert = _request->getClientCert();
            if (!clientCert.empty()) {
                builder.setCertFile(clientCert);
            }
            sslOption = builder.build();
        }
        SSLClientEndpoint endpoint(_client->reactor(), host, std::to_string(port), std::move(sslOption),
                                   _request->getConnectTimeout());
        connectDeferred = connectProtocol(endpoint, shared_from_this());
    } else {
        TCPClientEndpoint endpoint(_client->reactor(), host, std::to_string(port),
                                   _request->getConnectTimeout());
        connectDeferred = connectProtocol(endpoint, shared_from_this());
    }
    connectDeferred->addErrback([this, self = shared_from_this()](DeferredValue value) {
        try {
            value.throwError();
        } catch (TimeoutError &error) {
            onTimeout("while connecting");
        }
        return value;
    })->addErrback([this, self = shared_from_this()](DeferredValue value) {
        handleException(value.asError());
        return value;
    });
}

void HTTPClientConnection::writeHeaders(const RequestStartLine &startLine, HTTPHeaders &headers) {
    StringVector lines;
    lines.emplace_back(StrUtil::format("%s %s HTTP/1.1", startLine.getMethod(), startLine.getPath()));
    _requestStartLine = startLine;
    _chunkingOutput = (startLine.getMethod() == "POST" ||
                       startLine.getMethod() == "PUT" ||
                       startLine.getMethod() == "PATCH") &&
                      !headers.has("Content-Length") &&
                      !headers.has("Transfer-Encoding");
    if (_chunkingOutput) {
        headers["Transfer-Encoding"] = "chunked";
    }
    if (headers.has("Content-Length")) {
        _expectedContentRemaining = std::stoi(headers.at("Content-Length"));
    } else {
        _expectedContentRemaining = boost::none;
    }
    headers.getAll([&lines](const std::string &name, const std::string &value) {
        lines.emplace_back(name + ": " + value);
    });
    for (auto &line: lines) {
        if (line.find('\n') != std::string::npos) {
            NET4CXX_THROW_EXCEPTION(ValueError, "Newline in header: %s", line);
        }
    }
    auto data = boost::join(lines, "\r\n") + "\r\n\r\n";
    write(data, true);
}

void HTTPClientConnection::readResponse() {
    _state = READ_HEADER;
    readUntilRegex("\r?\n\r?\n", _maxHeaderSize);
}

void HTTPClientConnection::readBody() {
    boost::optional<size_t> contentLength;
    if (_headers->has("Content-Length")) {
        if (_headers->has("Transfer-Encoding")) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "Response with both Transfer-Encoding and Content-Length");
        }
        if (_headers->at("Content-Length").find(',') != std::string::npos) {
            StringVector pieces = StrUtil::split(_headers->at("Content-Length"), ',');
            for (auto &piece: pieces) {
                boost::trim(piece);
            }
            for (auto &piece: pieces) {
                if (piece != pieces[0]) {
                    NET4CXX_THROW_EXCEPTION(HTTPInputError, "Multiple unequal Content-Lengths: %s",
                                            _headers->at("Content-Length"));
                }
            }
            (*_headers)["Content-Length"] = pieces[0];
        }
        try {
            contentLength = (size_t)std::stoul(_headers->at("Content-Length"));
        } catch (...) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "Only integer Content-Length is allowed: %s",
                                    _headers->at("Content-Length"));
        }
    }
    if (_responseStartLine.getCode() == 204) {
        if (_headers->has("Transfer-Encoding") || (contentLength && *contentLength != 0)) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "Response with code 204 should not have body");
        }
    }
    if (contentLength) {
        readFixedBody(*contentLength);
        return;
    }
    if (boost::to_lower_copy(_headers->get("Transfer-Encoding", "")) == "chunked") {
        readChunkLength();
        return;
    }
    _state = READ_UNTIL_CLOSE;
    readUntilClose();
}

void HTTPClientConnection::writeBody(bool startRead) {
    if (!_request->getBody().empty()) {
        writeChunk(_request->getBody());
        writeFinished();
    } else if (_request->getBodyProducer()) {
        _request->getBodyProducer()->active(getSelf<HTTPClientConnection>());
    }
    if (startRead) {
        readResponse();
    }
}

void HTTPClientConnection::onTimeout(const std::string &info) {
    _timeout.reset();
    if (_callback) {
        std::string errorMessage;
        if (info.empty()) {
            errorMessage = "Timeout";
        } else {
            errorMessage = "Timeout " + info;
        }
        NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(599) << errinfo_http_reason(errorMessage);
    }
}

void HTTPClientConnection::removeTimeout() {
    if (!_timeout.cancelled()) {
        _timeout.cancel();
        _timeout.reset();
    }
}

void HTTPClientConnection::runCallback(HTTPResponse response) {
    if (_callback) {
        CallbackType callback(std::move(_callback));
        _callback = nullptr;
        _client->reactor()->addCallback([callback = std::move(callback), response = std::move(response)]() {
            callback(response);
        });
    }
}

void HTTPClientConnection::handleException(std::exception_ptr error) {
    if (_callback) {
        removeTimeout();
        try {
            std::rethrow_exception(error);
        } catch (StreamClosedError &e) {
            error = std::make_exception_ptr(
                    NET4CXX_MAKE_EXCEPTION(HTTPError, "") << errinfo_http_code(599)
                                                          << errinfo_http_reason("Stream closed"));
        } catch (...) {

        }
        runCallback(HTTPResponse(_request, 599, error, TimestampClock::now() - _startTime));
        if (_connected) {
            close(nullptr);
        }
    }
}

void HTTPClientConnection::onHeaders(char *data, size_t length) {
    std::string startLine;
    std::shared_ptr<HTTPHeaders> headers;
    std::tie(startLine, headers) = HTTPUtil::parseHeaders(data, length);
    _responseStartLine = HTTPUtil::parseResponseStartLine(startLine);
    onHeadersReceived(_responseStartLine, headers);
    bool skipBody = false;
    if (_requestStartLine.getMethod() == "HEAD") {
        skipBody = true;
    }
    if (_responseStartLine.getCode() == 304) {
        skipBody = true;
    }
    if (_responseStartLine.getCode() >= 100 && _responseStartLine.getCode() < 200) {
        if (headers->has("Content-Length") || headers->has("Transfer-Encoding")) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "Response code %d cannot have body", _responseStartLine.getCode());
        }
        readResponse();
        return;
    }
    if (_request->getDecompressResponse() && _headers->get("Content-Encoding") == "gzip") {
        _decompressor = std::make_unique<GzipDecompressor>();
        _headers->add("X-Consumed-Content-Encoding", _headers->at("Content-Encoding"));
        _headers->erase("Content-Encoding");
    }
    if (!skipBody) {
        readBody();
    } else {
        finish();
    }
}

void HTTPClientConnection::onFixedBody(char *data, size_t length) {
    onDataReceived(data, length);
    if (_bytesRead < _bytesToRead) {
        readFixedBodyBlock();
    } else {
        finish();
    }
}

void HTTPClientConnection::onChunkLength(char *data, size_t length) {
    std::string content(data, length);
    boost::trim(content);
    size_t chunkLen = std::stoul(content, nullptr, 16);
    if (chunkLen == 0) {
        readLastChunkEnds();
    } else {
        if (chunkLen + _totalSize > _maxBodySize) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "chunked body too large");
        }
        readChunkData(chunkLen);
    }
}

void HTTPClientConnection::onChunkData(char *data, size_t length) {
    onDataReceived(data, length);
    if (_bytesRead < _bytesToRead) {
        readChunkDataBlock();
    } else {
        readChunkEnds();
    }
}

void HTTPClientConnection::onChunkEnds(char *data, size_t length) {
    NET4CXX_ASSERT_THROW(boost::string_view(data, length) == "\r\n", "");
    readChunkLength();
}

void HTTPClientConnection::onLastChunkEnds(char *data, size_t length) {
    if (boost::string_view(data, length) != "\r\n") {
        NET4CXX_THROW_EXCEPTION(HTTPInputError, "improperly terminated chunked request");
    }
    finish();
}

void HTTPClientConnection::onReadUntilClose(char *data, size_t length) {
    onDataReceived(data, length);
    finish();
}

void HTTPClientConnection::finish() {
    if (_decompressor) {
        auto tail = _decompressor->flushToString();
        if (!tail.empty()) {
            onChunkReceived(std::move(tail));
        }
    }
    _state = READ_NONE;

    auto data = boost::join(_chunks, "");
    removeTimeout();
    auto originalRequest = _request->getOriginalRequest();
    if (!originalRequest) {
        originalRequest = _request;
    }
    if (shouldFollowRedirect()) {
        auto newRequest = _request->clone();
        std::string url = _request->getUrl();
        url = UrlParse::urlJoin(url, _headers->at("Location"));
        newRequest->setUrl(std::move(url));
        newRequest->setMaxRedirects(_request->getMaxRedirects() - 1);
        newRequest->headers().erase("Host");
        if (_code == 302 || _code == 303) {
            newRequest->setMethod("GET");
            newRequest->setBody("");
            const std::array<std::string, 4> fields = {{
                                                               "Content-Length", "Content-Type", "Content-Encoding", "Transfer-Encoding"
                                                       }};
            for (auto &field: fields) {
                try {
                    newRequest->headers().erase(field);
                } catch (KeyError &e) {

                }
            }
        }
        newRequest->setOriginalRequest(std::move(originalRequest));
        CallbackType callback;
        callback = std::move(_callback);
        _callback = nullptr;
        _client->fetch(std::move(newRequest), std::move(callback));
        onEndRequest();
        return;
    }
    std::string buffer;
    auto &streamingCallback = _request->getStreamingCallback();
    if (streamingCallback) {

    } else {
        buffer = std::move(data);
    }
    HTTPResponse response(originalRequest, _code.get(), _reason, _headers, std::move(buffer), _request->getUrl(),
                          TimestampClock::now() - _startTime);
    runCallback(std::move(response));
    onEndRequest();
}

void HTTPClientConnection::onHeadersReceived(const ResponseStartLine &firstLine,
                                             const std::shared_ptr<HTTPHeaders> &headers) {
    if (_request->getExpect100Continue() && firstLine.getCode() == 100) {
        writeBody(false);
        return;
    }
    _code = firstLine.getCode();
    _reason = firstLine.getReason();
    _headers = headers;

    if (shouldFollowRedirect()) {
        return;
    }

    auto &headerCallback = _request->getHeaderCallback();
    if (headerCallback) {
        headerCallback(StrUtil::format("%s %d %s\r\n", firstLine.getVersion(), firstLine.getCode(),
                                       firstLine.getReason()));
        _headers->getAll([&headerCallback](const std::string &k, const std::string &v) {
            headerCallback(k + ": " + v + "\r\n");
        });
        headerCallback("\r\n");
    }
}

void HTTPClientConnection::onDataReceived(char *data, size_t length) {
    _bytesRead += length;
    _totalSize += length;
    if (_decompressor) {
        std::string compressed;
        compressed = _decompressor->decompressToString((const Byte *) data, length, _chunkSize);
        onChunkReceived(std::move(compressed));
        while (!_decompressor->getUnconsumedTail().empty()) {
            compressed = _decompressor->decompressToString(_decompressor->getUnconsumedTail(), _chunkSize);
            onChunkReceived(std::move(compressed));
        }
    } else {
        onChunkReceived(std::string{data, data + length});
    }
}

void HTTPClientConnection::onChunkReceived(std::string chunk) {
    if (shouldFollowRedirect()) {
        return;
    }
    auto &streamingCallback = _request->getStreamingCallback();
    if (streamingCallback) {
        streamingCallback(std::move(chunk));
    } else {
        _chunks.emplace_back(std::move(chunk));
    }
}

std::string HTTPClientConnection::formatChunk(const Byte *data, size_t length) {
    if (_expectedContentRemaining) {
        _expectedContentRemaining = *_expectedContentRemaining - length;
        if (*_expectedContentRemaining < 0) {
            try {
                NET4CXX_THROW_EXCEPTION(HTTPOutputError, "Tried to write more data than Content-Length");
            } catch (...) {
                close(std::current_exception());
                throw;
            }
        }
    }
    if (_chunkingOutput && length != 0) {
        std::string chunk;
        chunk = StrUtil::format("%x\r\n", length);
        chunk.append((const char *) data, length);
        chunk.append("\r\n");
        return chunk;
    } else {
        return std::string{(const char *) data, length};
    }
}

NS_END