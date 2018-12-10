//
// Created by yuwenyong.vincent on 2018/8/4.
//

#include "net4cxx/plugins/web/httpserver.h"
#include <boost/utility/string_view.hpp>
#include "net4cxx/core/network/ssl.h"
#include "net4cxx/plugins/web/web.h"


NS_BEGIN


void HTTPConnection::onConnected() {
    auto address = getRemoteAddress();
    if (NetUtil::isValidIP(address)) {
        _remoteIp = address;
    } else {
        _remoteIp = "0.0.0.0";
    }
    auto webApp = getFactory<WebApp>();
    if (!webApp->getProtocol().empty()) {
        _protocol = webApp->getProtocol();
    } else if (std::dynamic_pointer_cast<SSLConnection>(_transport)) {
        _protocol = "https";
    } else {
        _protocol = "http";
    }
    _origRemoteIp = _remoteIp;
    _origProtocol = _protocol;

    _noKeepAlive = webApp->getNoKeepAlive();
    _xheaders = webApp->getXHeaders();
    _decompress = webApp->getDecompressRequest();
    _chunkSize = webApp->getChunkSize() != 0 ? webApp->getChunkSize() : 65536;
    _maxHeaderSize = webApp->getMaxHeaderSize() != 0 ? webApp->getMaxHeaderSize() : 65536;
    _maxBodySize = webApp->getMaxBodySize() != 0 ? webApp->getMaxBodySize() : _maxBufferSize;
    _headerTimeout = webApp->getIdleConnectionTimeout();
    _bodyTimeout = webApp->getBodyTimeout();

    startRequest();
}

void HTTPConnection::onDataRead(Byte *data, size_t length) {
    try {
        try {
            if (_state == READ_HEADER) {
                onHeaders((char *)data, length);
            } else if (_state == READ_FIXED_BODY) {
                onFixedBody((char *)data, length);
            } else if (_state == READ_CHUNK_LENGTH) {
                onChunkLength((char *)data, length);
            } else if (_state == READ_CHUNK_DATA) {
                onChunkData((char *)data, length);
            } else if (_state == READ_CHUNK_ENDS) {
                onChunkEnds((char *)data, length);
            } else {
                NET4CXX_ASSERT_MSG(false, "Unreachable");
            }
        } catch (HTTPInputError &e) {
            NET4CXX_LOG_INFO(gGenLog, "Malformed HTTP request from %s: %s", _remoteIp, e.what());
            throw;
        } catch (std::exception &e) {
            NET4CXX_LOG_INFO(gGenLog, "Uncaught exception from %s: %s", _remoteIp, e.what());
            throw;
        } catch (...) {
            NET4CXX_LOG_INFO(gGenLog, "Unknown exception from %s", _remoteIp);
            throw;
        }
    } catch (...) {
        close(std::current_exception());
    }
}

void HTTPConnection::onWriteComplete() {
    _pendingWrite = false;
    if (_writeCallback) {
        WriteCallbackType callback = std::move(_writeCallback);
        _writeCallback = nullptr;
        try {
            callback();
        } catch (std::exception &e) {
            NET4CXX_LOG_INFO(gGenLog, "Uncaught exception in write callback from %s: %s", _remoteIp, e.what());
        } catch (...) {
            NET4CXX_LOG_INFO(gGenLog, "Unknown exception in write callback from %s", _remoteIp);
        }
    }
    if (_writeFinished) {
        finishRequest();
    }
}

void HTTPConnection::onDisconnected(std::exception_ptr reason) {
    if (_closeCallback) {
        CloseCallbackType callback = std::move(_closeCallback);
        _closeCallback = nullptr;
        try {
            callback();
        } catch (std::exception &e) {
            NET4CXX_LOG_INFO(gGenLog, "Uncaught exception in close callback from %s: %s", _remoteIp, e.what());
        } catch (...) {
            NET4CXX_LOG_INFO(gGenLog, "Unknown exception in close callback from %s", _remoteIp);
        }
    }
    if (_dispatcher) {
        _dispatcher->onConnectionClose();
    }
    clearCallbacks();
}

void HTTPConnection::writeHeaders(ResponseStartLine startLine, HTTPHeaders &headers, const Byte *chunk, size_t length,
                                  WriteCallbackType callback) {
    StringVector lines;
    lines.emplace_back(StrUtil::format("HTTP/1.1 %d %s", startLine.getCode(), startLine.getReason()));
    _responseStartLine = std::move(startLine);
    _chunkingOutput = _requestStartLine.getVersion() == "HTTP/1.1" &&
                      _responseStartLine.getCode() != 204 &&
                      _responseStartLine.getCode() != 304 &&
                      !headers.has("Content-Length") &&
                      !headers.has("Transfer-Encoding");
    if (_requestStartLine.getVersion() == "HTTP/1.0" &&
        boost::to_lower_copy(_requestHeaders->get("Connection")) == "keep-alive") {
        headers["Connection"] = "Keep-Alive";
    }
    if (_chunkingOutput) {
        headers["Transfer-Encoding"] = "chunked";
    }
    if (_requestStartLine.getMethod() == "HEAD" || _responseStartLine.getCode() == 304) {
        _expectedContentRemaining = 0;
    } else if (headers.has("Content-Length")) {
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
    if (length != 0) {
        data.append(formatChunk(chunk, length));
    }
    if (callback) {
        _writeCallback = std::move(callback);
    }
    _pendingWrite = true;
    write(data, true);
}

void HTTPConnection::writeChunk(const Byte *chunk, size_t length, WriteCallbackType callback) {
    if (callback) {
        _writeCallback = std::move(callback);
    }
    write(formatChunk(chunk, length), true);
}

void HTTPConnection::finish() {
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
        _pendingWrite = true;
        write("0\r\n\r\n", true);
    }
    _writeFinished = true;
    if (!_readFinished) {
        _disconnectOnFinish = true;
    }
    setNoDelay(true);
    if (!_pendingWrite) {
        finishRequest();
    }
}

void HTTPConnection::close(std::exception_ptr reason) {
    IOStream::close(reason);
    clearCallbacks();
    if (_headerTimeoutCall.active()) {
        _headerTimeoutCall.cancel();
    }
    if (_bodyTimeoutCall.active()) {
        _bodyTimeoutCall.cancel();
    }
}

void HTTPConnection::startRequest() {
    _totalSize = 0;
    _disconnectOnFinish = false;
    clearCallbacks();
    _chunkingOutput = false;
    _pendingWrite = false;
    _readFinished =false;
    _writeFinished = false;
    _expectedContentRemaining = boost::none;
    _decompressor.reset();
    _dispatcher = getFactory<WebApp>()->startRequest(getSelf<HTTPConnection>());
    readHeaders();
}

std::string HTTPConnection::formatChunk(const Byte *data, size_t length) {
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
        chunk.append((const char *)data, length);
        chunk.append("\r\n");
        return chunk;
    } else {
        return std::string{(const char *)data, length};
    }
}

void HTTPConnection::readHeaders() {
    _state = READ_HEADER;
    readUntilRegex("\r?\n\r?\n", _maxHeaderSize);
    if (_headerTimeout != 0.0) {
        _headerTimeoutCall = reactor()->callLater(_headerTimeout, [this, self=shared_from_this()]() {
            try {
                NET4CXX_THROW_EXCEPTION(TimeoutError, "Read header timeout");
            } catch (...) {
                close(std::current_exception());
            }
        });
    }
}

void HTTPConnection::readBody() {
    if (_requestHeaders->has("Content-Length")) {
        if (_requestHeaders->has("Transfer-Encoding")) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "Response with both Transfer-Encoding and Content-Length");
        }
        if (_requestHeaders->get("Content-Length").find(',') != std::string::npos) {
            StringVector pieces = StrUtil::split(_requestHeaders->at("Content-Length"), ',');
            for (auto &piece: pieces) {
                boost::trim(piece);
            }
            for (auto &piece: pieces) {
                if (piece != pieces[0]) {
                    NET4CXX_THROW_EXCEPTION(HTTPInputError, "Multiple unequal Content-Lengths: %s",
                                            _requestHeaders->at("Content-Length"));
                }
            }
            (*_requestHeaders)["Content-Length"] = pieces[0];
        }
        size_t contentLength;
        try {
            contentLength = (size_t)std::stoul(_requestHeaders->at("Content-Length"));
        } catch (...) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "Only integer Content-Length is allowed: %s",
                                    _requestHeaders->at("Content-Length"));
        }
        if (contentLength > _maxBodySize) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "Content-Length too long");
        }
        readFixedBody(contentLength);
        if (_bodyTimeout != 0.0) {
            _bodyTimeoutCall = reactor()->callLater(_bodyTimeout, [this, self=shared_from_this()]() {
                try {
                    NET4CXX_THROW_EXCEPTION(TimeoutError, "Read body timeout");
                } catch (...) {
                    close(std::current_exception());
                }
            });
        }
        return;
    }
    if (_requestHeaders->get("Transfer-Encoding") == "chunked") {
        readChunkLength();
        if (_bodyTimeout != 0.0) {
            _bodyTimeoutCall = reactor()->callLater(_bodyTimeout, [this, self=shared_from_this()]() {
                try {
                    NET4CXX_THROW_EXCEPTION(TimeoutError, "Read body timeout");
                } catch (...) {
                    close(std::current_exception());
                }
            });
        }
        return;
    }
    readFinished();
}

void HTTPConnection::onHeaders(char *data, size_t length) {
    if (_headerTimeoutCall.active()) {
        _headerTimeoutCall.cancel();
    }
    std::string startLine;
    std::tie(startLine, _requestHeaders) = HTTPUtil::parseHeaders(data, length);
    _requestStartLine = HTTPUtil::parseRequestStartLine(startLine);
    _disconnectOnFinish = !canKeepAlive(_requestStartLine, *_requestHeaders);
    onHeadersReceived();
    if (_requestHeaders->get("Expect") == "100-continue" && !_writeFinished) {
        write("HTTP/1.1 100 (Continue)\r\n\r\n");
    }
    readBody();
}

void HTTPConnection::onFixedBody(char *data, size_t length) {
    onDataReceived(data, length);
    if (_bytesRead < _bytesToRead) {
        readFixedBodyBlock();
    } else {
        readFinished();
    }
}

void HTTPConnection::onChunkLength(char *data, size_t length) {
    std::string content(data, length);
    boost::trim(content);
    size_t chunkLen = std::stoul(content, nullptr, 16);
    if (chunkLen == 0) {
        readFinished();
    } else {
        if (chunkLen + _totalSize > _maxBodySize) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "chunked body too large");
        }
        readChunkData(chunkLen);
    }
}

void HTTPConnection::onChunkData(char *data, size_t length) {
    onDataReceived(data, length);
    if (_bytesRead < _bytesToRead) {
        readChunkDataBlock();
    } else {
        readChunkEnds();
    }
}

void HTTPConnection::onChunkEnds(char *data, size_t length) {
    NET4CXX_ASSERT_THROW(boost::string_view(data, length) == "\r\n", "");
    readChunkLength();
}

void HTTPConnection::onHeadersReceived() {
    if (_decompress) {
        if (_requestHeaders->get("Content-Encoding") == "gzip") {
            _decompressor = std::make_unique<GzipDecompressor>();
            _requestHeaders->add("X-Consumed-Content-Encoding", _requestHeaders->at("Content-Encoding"));
            _requestHeaders->erase("Content-Encoding");
        }
    }
    if (_xheaders) {
        applyXheaders(*_requestHeaders);
    }
    _dispatcher->headersReceived(_requestStartLine, _requestHeaders);
}

void HTTPConnection::onDataReceived(char *data, size_t length) {
    _bytesRead += length;
    _totalSize += length;
    if (_decompressor) {
        std::string compressed;
        compressed = _decompressor->decompressToString((const Byte *)data, length, _chunkSize);
        if (!_writeFinished) {
            _dispatcher->dataReceived(std::move(compressed));
        }
        while (!_decompressor->getUnconsumedTail().empty()) {
            compressed = _decompressor->decompressToString(_decompressor->getUnconsumedTail(), _chunkSize);
            if (!_writeFinished) {
                _dispatcher->dataReceived(std::move(compressed));
            }
        }
    } else {
        if (!_writeFinished) {
            _dispatcher->dataReceived(std::string{data, data + length});
        }
    }
}

void HTTPConnection::readFinished() {
    _readFinished = true;
    if (_bodyTimeoutCall.active()) {
        _bodyTimeoutCall.cancel();
    }
    if (_decompressor) {
        auto tail = _decompressor->flushToString();
        if (!tail.empty()) {
            if (!_writeFinished) {
                _dispatcher->dataReceived(std::move(tail));
            }
        }
    }
    _state = READ_NONE;
    if (!_writeFinished) {
        _dispatcher->finish();
    }
}

bool HTTPConnection::canKeepAlive(const RequestStartLine &startLine, const HTTPHeaders &headers) {
    if (_noKeepAlive) {
        return false;
    }
    std::string connectionHeader = headers.get("Connection");
    if (!connectionHeader.empty()) {
        boost::to_lower(connectionHeader);
    }
    if (startLine.getVersion() == "HTTP/1.1") {
        return connectionHeader != "close";
    } else if (headers.has("Content-Length") ||
               boost::to_lower_copy(headers.get("Transfer-Encoding", "")) == "chunked" ||
               startLine.getMethod() == "HEAD" ||
               startLine.getMethod() == "GET") {
        return connectionHeader == "keep-alive";
    }
    return false;
}

void HTTPConnection::finishRequest() {
    if (_xheaders) {
        unapplyXheaders();
    }
    clearCallbacks();
    if (_disconnectOnFinish) {
        close(nullptr);
        return;
    }
    setNoDelay(false);
    startRequest();
}

void HTTPConnection::applyXheaders(const HTTPHeaders &headers) {
    auto ip = headers.get("X-Forwarded-For", _remoteIp);
    ip = boost::trim_copy(StrUtil::split(ip, ',').back());
    ip = headers.get("X-Real-Ip", ip);
    if (NetUtil::isValidIP(ip)) {
        _remoteIp = std::move(ip);
    }
    std::string proto = headers.get("X-Forwarded-Proto", _protocol);
    proto = headers.get("X-Scheme", proto);
    if (proto == "http" || proto == "https") {
        _protocol = std::move(proto);
    }
}

void HTTPConnection::unapplyXheaders() {
    _remoteIp = _origRemoteIp;
    _protocol = _origProtocol;
}


HTTPServerRequest::HTTPServerRequest(const std::shared_ptr<HTTPConnection> &connection,
                                     const RequestStartLine *startLine,
                                     const std::shared_ptr<HTTPHeaders> &headers,
                                     const std::string &method,
                                     const std::string &uri,
                                     const std::string &version,
                                     std::string body,
                                     std::string host,
                                     HTTPFileListMap files)
        : _method(startLine ? startLine->getMethod() : method)
        , _uri(startLine ? startLine->getPath() : uri)
        , _version(startLine ? startLine->getVersion() : version)
        , _headers(headers ? headers : std::make_shared<HTTPHeaders>())
        , _body(std::move(body))
        , _remoteIp(connection->getRemoteIp())
        , _protocol(connection->getProtocol())
        , _files(std::move(files))
        , _connection(connection)
        , _startTime(TimestampClock::now())
        , _finishTime(Timestamp::min()) {

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
            StringMap parsed;
            try {
                parsed = HTTPUtil::parseCookie(_headers->at("Cookie"));
            } catch (...) {

            }
            for (auto &kv: parsed) {
                try {
                    (*_cookies)[kv.first] = kv.second;
                } catch (...) {

                }
            }
        }
    }
    return _cookies.get();
}

void HTTPServerRequest::write(const Byte *chunk, size_t length, WriteCallbackType callback) {
    auto connection = getConnection();
    if (!connection) {
        NET4CXX_THROW_EXCEPTION(StreamClosedError, "Connection already closed");
    }
    NET4CXX_ASSERT_MSG(boost::starts_with(_version, "HTTP/1."), "deprecated interface only supported in HTTP/1.x");
    connection->writeChunk(chunk, length, std::move(callback));
}

void HTTPServerRequest::finish() {
    auto connection = getConnection();
    if (!connection) {
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

void HTTPServerRequest::parseBody() {
    HTTPUtil::parseBodyArguments(_headers->get("Content-Type", ""), _body, _bodyArguments, _files, _headers.get());
    for (const auto &kv: getBodyArguments()) {
        addArguments(kv.first, kv.second);
    }
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