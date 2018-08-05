//
// Created by yuwenyong.vincent on 2018/8/4.
//

#include "net4cxx/plugins/web/httpserver.h"


NS_BEGIN

HTTPServer::HTTPServer(RequestCallbackType requestCallback,
                       bool noKeepAlive,
                       Reactor *reactor,
                       bool xheaders,
                       std::string protocol,
                       SSLOptionPtr sslOption,
                       size_t maxBufferSize)
        : TCPServer(reactor, std::move(sslOption), maxBufferSize)
        , _requestCallback(std::move(requestCallback))
        , _noKeepAlive(noKeepAlive)
        , _xheaders(xheaders)
        , _protocol(std::move(protocol)) {

}

void HTTPServer::handleStream(BaseIOStreamPtr stream, std::string address) {
    auto connection = HTTPConnection::create(std::move(stream), std::move(address), _requestCallback, _noKeepAlive,
                                             _xheaders, _protocol);
    connection->start();
}


void HTTPConnection::start() {
    auto wrapper = std::make_shared<CloseCallbackWrapper>(shared_from_this());
    _stream->setCloseCallback(std::bind(&CloseCallbackWrapper::operator(), std::move(wrapper)));

    _stream->readUntil("\r\n\r\n", [this, self=shared_from_this()](ByteArray data) {
        onHeaders(std::move(data));
    });
}

void HTTPConnection::write(const Byte *chunk, size_t length, WriteCallbackType callback) {
    NET4CXX_ASSERT_MSG(!_requestObserver.expired(), "Request closed");
    if (!_stream->closed()) {
        _writeCallback = std::move(callback);
        if (_writeCallback) {
            auto wrapper = std::make_shared<WriteCallbackWrapper>(shared_from_this());
            _stream->write(chunk, length, std::bind(&WriteCallbackWrapper::operator(), std::move(wrapper)));
        } else {
            _stream->write(chunk, length, std::bind(&HTTPConnection::onWriteComplete, shared_from_this()));
        }
    }
}

void HTTPConnection::finish() {
    NET4CXX_ASSERT_MSG(!_requestObserver.expired(), "Request closed");
    _request = _requestObserver.lock();
    _requestFinished = true;
    _stream->setNoDelay(true);
    if (!_stream->writing()) {
        finishRequest();
    }
}

void HTTPConnection::onConnectionClose() {
    if (_closeCallback) {
        CloseCallbackType callback(std::move(_closeCallback));
        _closeCallback = nullptr;
        callback();
    }
    clearRequestState();
}

void HTTPConnection::onWriteComplete() {
    if (_writeCallback) {
        WriteCallbackType callback(std::move(_writeCallback));
        _writeCallback = nullptr;
        callback();
    }
    if (_requestFinished && !_stream->writing()) {
        finishRequest();
    }
}

void HTTPConnection::finishRequest() {
    bool disconnect;
    if (_noKeepAlive || !_request) {
        disconnect = true;
    } else {
        auto headers = _request->getHTTPHeaders();
        std::string connectionHeader = headers->get("Connection");
        if (!connectionHeader.empty()) {
            boost::to_lower(connectionHeader);
        }
        if (_request->supportsHTTP11()) {
            disconnect = connectionHeader == "close";
        } else if (headers->has("Content-Length")
                   || _request->getMethod() == "HEAD"
                   || _request->getMethod() == "GET") {
            disconnect = connectionHeader != "keep-alive";
        } else {
            disconnect = true;
        }
    }
    clearRequestState();
    if (disconnect) {
        close();
        return;
    }
    try {
        _stream->readUntil("\r\n\r\n", [this, self=shared_from_this()](ByteArray data) {
            onHeaders(std::move(data));
        });
        _stream->setNoDelay(false);
    } catch (StreamClosedError &e) {
        close();
    }
}

void HTTPConnection::onHeaders(ByteArray data) {
    try {
        const char *eol = StrNStr((char *)data.data(), data.size(), "\r\n");
        std::string startLine, rest;
        if (eol) {
            startLine.assign((const char *)data.data(), eol);
            rest.assign(eol, (const char *)data.data() + data.size());
        } else {
            startLine.assign((char *)data.data(), data.size());
        }
        StringVector requestLineComponents = StrUtil::split(startLine);
        if (requestLineComponents.size() != 3) {
            NET4CXX_THROW_EXCEPTION(BadRequestException, "Malformed HTTP request line");
        }
        std::string method = std::move(requestLineComponents[0]);
        std::string uri = std::move(requestLineComponents[1]);
        std::string version = std::move(requestLineComponents[2]);
        if (!boost::starts_with(version, "HTTP/")) {
            NET4CXX_THROW_EXCEPTION(BadRequestException, "Malformed HTTP version in HTTP Request-Line");
        }
        std::unique_ptr<HTTPHeaders> headers;
        try {
            headers = HTTPHeaders::parse(rest);
        } catch (Exception &e) {
            NET4CXX_THROW_EXCEPTION(BadRequestException, "Malformed HTTP headers");
        }
        _request = HTTPServerRequest::create(shared_from_this(), std::move(method), std::move(uri), std::move(version),
                                             std::move(headers), std::string(), _address, _protocol);
        _requestObserver = _request;
        auto requestHeaders = _request->getHTTPHeaders();
        std::string contentLengthValue = requestHeaders->get("Content-Length");
        if (!contentLengthValue.empty()) {
            auto contentLength = (size_t)std::stoi(contentLengthValue);
            if (contentLength > _stream->getMaxBufferSize()) {
                NET4CXX_THROW_EXCEPTION(BadRequestException, "Content-Length too long");
            }
            if (requestHeaders->get("Expect") == "100-continue") {
                const char *continueLine = "HTTP/1.1 100 (Continue)\r\n\r\n";
                _stream->write((const Byte *)continueLine, strlen(continueLine));
            }
            _stream->readBytes(contentLength, [this, self=shared_from_this()](ByteArray data) {
                onRequestBody(std::move(data));
            });
            return;
        }
        _request->setConnection(shared_from_this());
        _requestCallback(std::move(_request));
    } catch (BadRequestException &e) {
        NET4CXX_LOG_INFO(gGenLog, "Malformed HTTP request from %s: %s", _address.c_str(), e.what());
        close();
    }
}

void HTTPConnection::onRequestBody(ByteArray data) {
    _request->setBody(std::string((const char *)data.data(), data.size()));
    auto headers = _request->getHTTPHeaders();
    const std::string &method = _request->getMethod();
    if (method == "POST" || method == "PATCH" || method == "PUT") {
        HTTPUtil::parseBodyArguments(headers->get("Content-Type", ""), _request->getBody(), _request->bodyArguments(),
                                     _request->files());
        for (const auto &kv: _request->getBodyArguments()) {
            _request->addArguments(kv.first, kv.second);
        }
    }
    _request->setConnection(shared_from_this());
    _requestCallback(std::move(_request));
}


HTTPServerRequest::HTTPServerRequest(HTTPConnectionPtr connection,
                                     std::string method,
                                     std::string uri,
                                     std::string version,
                                     std::unique_ptr<HTTPHeaders> &&headers,
                                     std::string body,
                                     std::string remoteIp,
                                     std::string protocol,
                                     std::string host,
                                     HTTPFileListMap files)
        : _method(std::move(method))
        , _uri(std::move(uri))
        , _version(std::move(version))
        , _headers(std::move(headers))
        , _body(std::move(body))
        , _files(std::move(files))
//        , _connection(std::move(connection))
        , _startTime(TimestampClock::now())
        , _finishTime(Timestamp::min()) {
    _remoteIp = std::move(remoteIp);
    if (!protocol.empty()) {
        _protocol = std::move(protocol);
    } else if (connection && std::dynamic_pointer_cast<SSLIOStream>(connection->getStream())) {
        _protocol = "https";
    } else {
        _protocol = "http";
    }
    if (connection && connection->getXHeaders()) {
        std::string ip = _headers->get("X-Forwarded-For", _remoteIp);
        ip = boost::trim_copy(StrUtil::split(ip, ',').back());
        ip = _headers->get("X-Real-Ip", ip);
        if (NetUtil::isValidIP(ip)) {
            _remoteIp = std::move(ip);
        }
        std::string proto = _headers->get("X-Forwarded-Proto", _protocol);
        proto = _headers->get("X-Scheme", proto);
        if (proto == "http" || proto == "https") {
            _protocol = std::move(proto);
        }
    }
    if (!host.empty()) {
        _host = std::move(host);
    } else {
        _host = _headers->get("Host", "127.0.0.1");
    }
    std::tie(_path, std::ignore, _query) = StrUtil::partition(_uri, "?");
    _arguments = UrlParse::parseQS(_query, true);
    _queryArguments = _arguments;
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->inc(WatchKeys::HTTPServerRequestCount);
#endif
}

const SimpleCookie& HTTPServerRequest::cookies() const {
    if (!_cookies) {
        _cookies.emplace();
        if (_headers->has("Cookie")) {
            try {
                _cookies->load(_headers->at("Cookie"));
            } catch (...) {
                _cookies->clear();
            }
        }
    }
    return _cookies.get();
}

void HTTPServerRequest::write(const Byte *chunk, size_t length, WriteCallbackType callback) {
    NET4CXX_ASSERT(_connection);
    _connection->write(chunk, length, std::move(callback));
}

void HTTPServerRequest::finish() {
    NET4CXX_ASSERT(_connection);
    auto connection = std::move(_connection);
    connection->finish();
    _finishTime = TimestampClock::now();
}

double HTTPServerRequest::requestTime() const {
    std::chrono::microseconds elapse;
    if (_finishTime == Timestamp::min()) {
        elapse = std::chrono::duration_cast<std::chrono::microseconds>(TimestampClock::now() - _startTime);
    } else {
        elapse = std::chrono::duration_cast<std::chrono::microseconds>(_finishTime - _startTime);
    }
    return elapse.count() / 1000000 + elapse.count() % 1000000 / 1000000.0;
}


std::ostream& operator<<(std::ostream &os, const HTTPServerRequest &request) {
    StringVector argsList;
    argsList.emplace_back("protocol=" + request.getProtocol());
    argsList.emplace_back("host=" + request.getHost());
    argsList.emplace_back("method=" + request.getMethod());
    argsList.emplace_back("uri=" + request.getURI());
    argsList.emplace_back("version=" + request.getVersion());
    argsList.emplace_back("remoteIp=" + request.getRemoteIp());
//    argsList.emplace_back("body=" + request.getBody());
    std::string args = boost::join(argsList, ",");
    StringVector headersList;
    request.getHTTPHeaders()->getAll([&headersList](const std::string &name, const std::string &value){
        headersList.emplace_back("\"" + name + "\": \"" + value + "\"");
    });
    std::string headers = boost::join(headersList, ", ");
    os << StrUtil::format("HTTPRequest(%s, headers=\"{%s}\"", args.c_str(), headers.c_str());
    return os;
}

NS_END