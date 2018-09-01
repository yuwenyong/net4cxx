//
// Created by yuwenyong.vincent on 2018/7/22.
//

#include "net4cxx/plugins/web/httpclient.h"
#include "net4cxx/common/crypto/base64.h"


NS_BEGIN


std::ostream& operator<<(std::ostream &os, const HTTPResponse &response) {
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


const StringSet HTTPClientConnection::_SUPPORTED_METHODS = {"GET", "HEAD", "POST", "PUT", "DELETE", "PATCH", "OPTIONS"};

void HTTPClientConnection::start() {
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
        const boost::regex hostPort(R"(^(.+):(\d+)$)");
        boost::smatch match;
        std::string host;
        unsigned short port;
        if (boost::regex_match(netloc, match, hostPort)) {
            host = match[1];
            port = (unsigned short)std::stoul(match[2]);
        } else {
            host = netloc;
            port = (unsigned short)(scheme == "https" ? 443 : 80);
        }
        const boost::regex ipv6(R"(^\[.*\]$)");
        if (boost::regex_match(host, ipv6)) {
            host = host.substr(1, host.size() - 2);
        }
        _parsedHostname = host;
        double timeout = std::min(_request->getConnectTimeout(), _request->getRequestTimeout());
        if (timeout != 0.0) {
            _timeout = _reactor->callLater(timeout, [this, self=shared_from_this()]() {
                try {
                    onTimeout();
                } catch (...) {
                    handleException(std::current_exception());
                }
            });
        }
        const StringMap &hostnameMapping = _client->getHostnameMapping();
        auto iter = hostnameMapping.find(host);
        if (iter != hostnameMapping.end()) {
            host = iter->second;
        }
        _resolver.async_resolve(host, std::to_string(port),
                                [this, self = shared_from_this()](const boost::system::error_code &ec,
                                                                  BaseIOStream::ResolverResultsType results) {
                                    try {
                                        onResolve(ec, std::move(results));
                                    } catch (...) {
                                        handleException(std::current_exception());
                                    }
                                });
    } catch (...) {
        handleException(std::current_exception());
    }
}

void HTTPClientConnection::onResolve(const boost::system::error_code &ec, BaseIOStream::ResolverResultsType addresses) {
    if (ec) {
        throw boost::system::system_error(ec);
    }
    if (!_callback) {
        return;
    }
    _stream = createStream();
    _stream->setCloseCallback([this, self=shared_from_this()]() {
        try {
            onClose();
        } catch (...) {
            handleException(std::current_exception());
        }
    });
    _stream->connect(std::move(addresses), [this, self=shared_from_this()]() {
        try {
            onConnect();
        } catch (...) {
            handleException(std::current_exception());
        }
    });
}

std::shared_ptr<BaseIOStream> HTTPClientConnection::createStream() const {
    BaseIOStream::SocketType socket(_reactor->getIOContext());
    if (_parsed.getScheme() == "https") {
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
        auto sslOption = builder.build();
        return SSLIOStream::create(std::move(socket), std::move(sslOption), _reactor, _maxBufferSize);
    } else {
        return IOStream::create(std::move(socket), _reactor, _maxBufferSize);
    }
}

void HTTPClientConnection::onTimeout() {
    _timeout.reset();
    if (_callback) {
        NET4CXX_THROW_EXCEPTION(HTTPError, "Timeout") << errinfo_http_code(599);
    }
}

void HTTPClientConnection::removeTimeout() {
    if (!_timeout.cancelled()) {
        _timeout.cancel();
        _timeout.reset();
    }
}

void HTTPClientConnection::onConnect() {
    removeTimeout();
    if (!_callback) {
        return;
    }
    double requestTimeout = _request->getRequestTimeout();
    if (requestTimeout != 0.0) {
        _timeout = _reactor->callLater(requestTimeout, [this, self=shared_from_this()]() {
            try {
                onTimeout();
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
        if (method == "POST" || method == "PATCH" || method == "PUT") {
            NET4CXX_ASSERT_THROW(!requestBody.empty(), "Body must not be empty for \"%s\" request", method);
        } else {
            NET4CXX_ASSERT_THROW(requestBody.empty(), "Body must be empty for \"%s\" request", method);
        }
    }
    if (!requestBody.empty()) {
        headers["Content-Length"] = std::to_string(requestBody.size());
    }
    if (method == "POST" && !headers.has("Content-Type")) {
        headers["Content-Type"] = "application/x-www-form-urlencoded";
    }
    if (_request->isUseGzip()) {
        headers["Accept-Encoding"] = "gzip";
    }
    const std::string &parsedPath = _parsed.getPath();
    const std::string &parsedQuery = _parsed.getQuery();
    std::string reqPath = parsedPath.empty() ? "/" : parsedPath;
    if (!parsedQuery.empty()) {
        reqPath += '?';
        reqPath += parsedQuery;
    }
    StringVector requestLines;
    requestLines.emplace_back(StrUtil::format("%s %s HTTP/1.1", method.c_str(), reqPath.c_str()));
    headers.getAll([&requestLines](const std::string &k, const std::string &v){
        std::string line = k + ": " + v;
        if (line.find('\n') != std::string::npos) {
            NET4CXX_THROW_EXCEPTION(ValueError, "Newline in header: %s", line);
        }
        requestLines.emplace_back(std::move(line));
    });
    std::string requestStr = boost::join(requestLines, "\r\n");
    requestStr += "\r\n\r\n";
    if (!requestBody.empty()) {
        requestStr += requestBody;
    }
    _stream->setNoDelay(true);
    _stream->write((const Byte *)requestStr.data(), requestStr.size());
    _stream->readUntilRegex("\r?\n\r?\n", [this, self=shared_from_this()](ByteArray data) {
        try {
            onHeaders(std::move(data));
        } catch (...) {
            handleException(std::current_exception());
        }
    });
}

void HTTPClientConnection::runCallback(HTTPResponse response) {
    if (_callback) {
        CallbackType callback(std::move(_callback));
        _callback = nullptr;
        _reactor->addCallback([callback=std::move(callback), response=std::move(response)]() {
            callback(std::move(response));
        });
    }
}

void HTTPClientConnection::handleException(std::exception_ptr error) {
    if (_callback) {
        removeTimeout();
        runCallback(HTTPResponse(_request, 599, error, TimestampClock::now() - _startTime));
        if (_stream) {
            _stream->close();
        }
    }
}

void HTTPClientConnection::onClose() {
    if (_callback) {
        std::string message("Connection closed");
        if (_stream->getError()) {
            try {
                std::rethrow_exception(_stream->getError());
            } catch (std::exception &e) {
                message = e.what();
            }
        }
        NET4CXX_THROW_EXCEPTION(HTTPError, message) << errinfo_http_code(599);
    }
}

void HTTPClientConnection::handle1xx(int code) {
    _stream->readUntilRegex("\r?\n\r?\n",[this, self=shared_from_this()](ByteArray data) {
        try {
            onHeaders(std::move(data));
        } catch (...) {
            handleException(std::current_exception());
        }
    });
}

void HTTPClientConnection::onHeaders(ByteArray data) {
    const char *content = (const char *)data.data();
    const char *eol = StrNStr(content, data.size(), "\n");
    std::string firstLine, _, headerData;
    if (eol) {
        firstLine.assign(content, eol);
        _ = "\n";
        headerData.assign(eol + 1, content + data.size());
    } else {
        firstLine.assign(content, data.size());
    }
    const boost::regex firstLinePattern("HTTP/1.[01] ([0-9]+) ([^\r]*).*");
    boost::smatch match;
    if (!boost::regex_match(firstLine, match, firstLinePattern)) {
        NET4CXX_THROW_EXCEPTION(Exception, "Unexpected first line");
    }
    int code = std::stoi(match[1]);
    _headers = HTTPHeaders::parse(headerData);
    if (code >= 100 && code < 200) {
        handle1xx(code);
        return;
    } else {
        _code = code;
        _reason = match[2];
    }
    boost::optional<size_t> contentLength;
    if (_headers->has("Content-Length")) {
        if (_headers->at("Content-Length").find(',') != std::string::npos) {
            StringVector pieces = StrUtil::split(_headers->at("Content-Length"), ',');
            for (auto &piece: pieces) {
                boost::trim(piece);
            }
            for (auto &piece: pieces) {
                if (piece != pieces[0]) {
                    NET4CXX_THROW_EXCEPTION(ValueError, "Multiple unequal Content-Lengths: %s",
                                            _headers->at("Content-Length"));
                }
            }
            (*_headers)["Content-Length"] = pieces[0];
        }
        contentLength = std::stoul(_headers->at("Content-Length"));
    }
    auto &headerCallback = _request->getHeaderCallback();
    if (headerCallback) {
        headerCallback(firstLine + _);
        _headers->getAll([&headerCallback](const std::string &k, const std::string &v){
            headerCallback(k + ": " + v + "\r\n");
        });
        headerCallback("\r\n");
    }
    if (_request->getMethod() == "HEAD" || _code == 304) {
        onBody({});
        return;
    }
    if ((100 <= _code && _code < 200) || _code == 204) {
        if (_headers->has("Transfer-Encoding") || (contentLength && *contentLength != 0)) {
            NET4CXX_THROW_EXCEPTION(ValueError, "Response with code %d should not have body", *_code);
        }
        onBody({});
        return;
    }
    if (_request->isUseGzip() && _headers->get("Content-Encoding") == "gzip") {
        _decompressor = std::make_unique<GzipDecompressor>();
    }
    if (_headers->get("Transfer-Encoding") == "chunked") {
        _chunks = ByteArray();
        _stream->readUntil("\r\n", [this, self=shared_from_this()](ByteArray data) {
            try {
                onChunkLength(std::move(data));
            } catch (...) {
                handleException(std::current_exception());
            }
        });
    } else if (contentLength) {
        _stream->readBytes(*contentLength, [this, self=shared_from_this()](ByteArray data) {
            try {
                onBody(std::move(data));
            } catch (...) {
                handleException(std::current_exception());
            }
        });
    } else {
        _stream->readUntilClose([this, self=shared_from_this()](ByteArray data) {
            try {
                onBody(std::move(data));
            } catch (...) {
                handleException(std::current_exception());
            }
        });
    }
}

void HTTPClientConnection::onBody(ByteArray data) {
    if (!_timeout.cancelled()) {
        _timeout.cancel();
        _timeout.reset();
    }
    auto originalRequest = _request->getOriginalRequest();
    if (!originalRequest) {
        originalRequest = _request;
    }
    if (_request->isFollowRedirects() && _request->getMaxRedirects() > 0
        && (_code == 301 || _code == 302 || _code == 303 || _code == 307)) {
        auto newRequest= _request->clone();
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
    if (_decompressor) {
        data = _decompressor->decompress(data);
        auto tail = _decompressor->flush();
        if (!tail.empty()) {
            data.insert(data.end(), tail.begin(), tail.end());
        }
    }
    std::string buffer;
    auto &streamingCallback = _request->getStreamingCallback();
    if (streamingCallback) {
        if (!_chunks) {
            streamingCallback(std::move(data));
        }
    } else {
        buffer.assign((const char*)data.data(), data.size());
    }
    HTTPResponse response(originalRequest, _code.get(), _reason, *_headers, std::move(buffer), _request->getUrl(),
                          TimestampClock::now() - _startTime);
    runCallback(std::move(response));
    onEndRequest();
}

void HTTPClientConnection::onChunkLength(ByteArray data) {
    std::string content((const char *)data.data(), data.size());
    boost::trim(content);
    size_t length = std::stoul(content, nullptr, 16);
    if (length == 0) {
        if (_decompressor) {
            auto tail = _decompressor->flush();
            if (!tail.empty()) {
                auto &streamingCallback = _request->getStreamingCallback();
                if (streamingCallback) {
                    streamingCallback(std::move(tail));
                } else {
                    _chunks->insert(_chunks->end(), tail.begin(), tail.end());
                }
            }
            _decompressor.reset();
        }
        onBody(std::move(_chunks.get()));
    } else {
        _stream->readBytes(length + 2, [this, self=shared_from_this()](ByteArray data) {
            try {
                onChunkData(std::move(data));
            } catch (...) {
                handleException(std::current_exception());
            }
        });
    }
}

void HTTPClientConnection::onChunkData(ByteArray data) {
    NET4CXX_ASSERT_THROW(strncmp((const char *)data.data() + data.size() - 2, "\r\n", 2) == 0, "");
    data.erase(std::prev(data.end(), 2), data.end());
    ByteArray chunk(std::move(data));
    if (_decompressor) {
        chunk = _decompressor->decompress(chunk);
    }
    auto &streamingCallback = _request->getStreamingCallback();
    if (streamingCallback) {
        streamingCallback(chunk);
    } else {
        _chunks->insert(_chunks->end(), chunk.begin(), chunk.end());
    }
    _stream->readUntil("\r\n", [this, self=shared_from_this()](ByteArray data) {
        try {
            onChunkLength(std::move(data));
        } catch (...) {
            handleException(std::current_exception());
        }
    });
}

NS_END