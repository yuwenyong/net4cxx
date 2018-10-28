//
// Created by yuwenyong.vincent on 2018/8/4.
//

#include "net4cxx/plugins/web/httpserver.h"
#include "net4cxx/core/network/ssl.h"
#include "net4cxx/plugins/web/web.h"


NS_BEGIN


void HTTPConnection::connectionMade() {
    _address = getRemoteAddress();
    auto webApp = getFactory<WebApp>();
    if (!webApp->getProtocol().empty()) {
        _protocol = webApp->getProtocol();
    } else if (std::dynamic_pointer_cast<SSLConnection>(_transport)) {
        _protocol = "https";
    } else {
        _protocol = "http";
    }

    _noKeepAlive = webApp->getNoKeepAlive();
    _xheaders = webApp->getXHeaders();

    _state = READ_HEADER;
    readUntil("\r\n\r\n");
}

void HTTPConnection::dataRead(Byte *data, size_t length) {
    if (_state == READ_HEADER) {
        onHeaders(data, length);
    } else if (_state == READ_BODY) {
        onRequestBody(data, length);
    } else {
        NET4CXX_ASSERT_MSG(false, "Unreachable");
    }
}

void HTTPConnection::connectionClose(std::exception_ptr reason) {
    if (_closeCallback) {
        CloseCallbackType callback = std::move(_closeCallback);
        _closeCallback = nullptr;
        callback();
    }
    clearRequestState();
}

void HTTPConnection::writeChunk(const Byte *chunk, size_t length) {
    NET4CXX_ASSERT_MSG(_request, "Request closed");
    if (!closed()) {
        _transport->write(chunk, length);
        onWriteComplete();
    }
}

void HTTPConnection::finish() {
    NET4CXX_ASSERT_MSG(_request, "Request closed");
    _requestFinished = true;
    setNoDelay(true);
    finishRequest();
}

void HTTPConnection::onHeaders(Byte *data, size_t length) {
    try {
        const char *eol = StrNStr((char *) data, length, "\r\n");
        std::string startLine, rest;
        if (eol) {
            startLine.assign((const char *) data, eol);
            rest.assign(eol, (const char *) data + length);
        } else {
            startLine.assign((char *) data, length);
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
        _request = HTTPServerRequest::create(getSelf<HTTPConnection>(), std::move(method), std::move(uri),
                                             std::move(version), std::move(headers), std::string(), _address,
                                             _protocol);
        auto requestHeaders = _request->getHTTPHeaders();
        std::string contentLengthValue = requestHeaders->get("Content-Length");
        if (!contentLengthValue.empty()) {
            auto contentLength = (size_t) std::stoi(contentLengthValue);
            if (contentLength > _maxBufferSize) {
                NET4CXX_THROW_EXCEPTION(BadRequestException, "Content-Length too long");
            }
            if (requestHeaders->get("Expect") == "100-continue") {
                const char *continueLine = "HTTP/1.1 100 (Continue)\r\n\r\n";
                write(continueLine);
            }
            _state = READ_BODY;
            readBytes(contentLength);
            return;
        }
        onProcessRequest();
    } catch (BadRequestException &e) {
        NET4CXX_LOG_INFO(gGenLog, "Malformed HTTP request from %s: %s", _address.c_str(), e.what());
        close();
    }
}

void HTTPConnection::onRequestBody(Byte *data, size_t length) {
    _request->setBody(std::string((const char *) data, length));
    auto headers = _request->getHTTPHeaders();
    const std::string &method = _request->getMethod();
    if (method == "POST" || method == "PATCH" || method == "PUT") {
        HTTPUtil::parseBodyArguments(headers->get("Content-Type", ""), _request->getBody(), _request->bodyArguments(),
                                     _request->files());
        for (const auto &kv: _request->getBodyArguments()) {
            _request->addArguments(kv.first, kv.second);
        }
    }
    onProcessRequest();
}

void HTTPConnection::onProcessRequest() {
    auto webApp = getFactory<WebApp>();
    _state = PROCESS_REQUEST;
    (*webApp)(_request);
}

void HTTPConnection::onWriteComplete() {
    if (_requestFinished) {
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
    _state = READ_HEADER;
    readUntil("\r\n");
    setNoDelay(false);
}


HTTPServerRequest::HTTPServerRequest(std::shared_ptr<HTTPConnection> connection,
                                     std::string method,
                                     std::string uri,
                                     std::string version,
                                     std::unique_ptr<HTTPHeaders> &&headers,
                                     std::string body,
                                     std::string remoteIp,
                                     std::string protocol,
                                     std::string host,
                                     HTTPFileListMap files)
        : _method(std::move(method)), _uri(std::move(uri)), _version(std::move(version)), _headers(std::move(headers)),
          _body(std::move(body)), _remoteIp(std::move(remoteIp)), _protocol(std::move(protocol)),
          _files(std::move(files)), _connection(connection), _startTime(TimestampClock::now()),
          _finishTime(Timestamp::min()) {

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

const SimpleCookie &HTTPServerRequest::cookies() const {
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

void HTTPServerRequest::write(const Byte *chunk, size_t length) {
    auto connection = getConnection();
    if (!connection || connection->closed()) {
        NET4CXX_THROW_EXCEPTION(StreamClosedError, "Connection already closed");
    }
    connection->writeChunk(chunk, length);
}

void HTTPServerRequest::finish() {
    auto connection = getConnection();
    if (!connection || connection->closed()) {
        NET4CXX_THROW_EXCEPTION(StreamClosedError, "Connection already closed");
    }
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


std::ostream &operator<<(std::ostream &os, const HTTPServerRequest &request) {
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
    request.getHTTPHeaders()->getAll([&headersList](const std::string &name, const std::string &value) {
        headersList.emplace_back("\"" + name + "\": \"" + value + "\"");
    });
    std::string headers = boost::join(headersList, ", ");
    os << StrUtil::format("HTTPRequest(%s, headers=\"{%s}\"", args.c_str(), headers.c_str());
    return os;
}

NS_END