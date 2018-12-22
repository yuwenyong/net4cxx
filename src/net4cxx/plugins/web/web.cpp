//
// Created by yuwenyong.vincent on 2018/8/11.
//

#include "net4cxx/plugins/web/web.h"
#include "net4cxx/common/crypto/hashlib.h"
#include "net4cxx/core/network/defer.h"


NS_BEGIN


StringVector MissingArgumentError::getCustomErrorInfo() const {
    auto errorInfo = HTTPError::getCustomErrorInfo();
    errorInfo.emplace_back("Missing argument " + getArgName());
    return errorInfo;
}


const boost::regex RequestHandler::_removeControlCharsRegex(R"([\x00-\x08\x0e-\x1f])");
const boost::regex RequestHandler::_invalidHeaderCharRe(R"([\x00-\x1f])");

RequestHandler::~RequestHandler() {
#ifdef NET4CXX_DEBUG
    NET4CXX_Watcher->dec(WatchKeys::RequestHandlerCount);
#endif
}

void RequestHandler::start(const ArgsType &args) {
    _request->getConnection()->setCloseCallback([this, self=shared_from_this()](){
        onConnectionClose();
    });
    initialize(args);
}

void RequestHandler::initialize(const ArgsType &args) {

}

const RequestHandler::SettingsType& RequestHandler::getSettings() const {
    return _application->getSettings();
}

DeferredPtr RequestHandler::onHead(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
    return nullptr;
}

DeferredPtr RequestHandler::onGet(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
    return nullptr;
}

DeferredPtr RequestHandler::onPost(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
    return nullptr;
}

DeferredPtr RequestHandler::onDelete(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
    return nullptr;
}

DeferredPtr RequestHandler::onPatch(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
    return nullptr;
}

DeferredPtr RequestHandler::onPut(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
    return nullptr;
}

DeferredPtr RequestHandler::onOptions(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
    return nullptr;
}

DeferredPtr RequestHandler::prepare() {
    return nullptr;
}

void RequestHandler::onFinish() {

}

void RequestHandler::onConnectionClose() {

}

bool RequestHandler::hasStreamRequestBody() const {
    return false;
}

void RequestHandler::dataReceived(std::string data) {

}

void RequestHandler::clear() {
    _headers = HTTPHeaders({
                                   {"Server", NET4CXX_VER },
                                   {"Content-Type", "text/html; charset=UTF-8"},
                                   {"Date", HTTPUtil::formatTimestamp(time(nullptr))},
                           });
    setDefaultHeaders();
//    _writeBuffer.clear();
    _statusCode = 200;
    _reason = HTTP_STATUS_CODES.at(200);
}

void RequestHandler::setDefaultHeaders() {

}

void RequestHandler::setStatus(int statusCode, const std::string &reason) {
    _statusCode = statusCode;
    if (!reason.empty()) {
        _reason = reason;
    } else {
        try {
            _reason = HTTP_STATUS_CODES.at(statusCode);
        } catch (std::out_of_range &e) {
            NET4CXX_THROW_EXCEPTION(ValueError, "unknown status code %d", statusCode);
        }
    }
}

void RequestHandler::setCookie(const std::string &name, const std::string &value, const std::string &domain,
                               const DateTime &expires, const std::string &path, boost::optional<int> expiresDays,
                               const StringMap &args) {
    boost::regex patt(R"([\x00-\x20])");
    if (boost::regex_search(name + value, patt)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "Invalid cookie %s: %s", name, value);
    }
    if (!_newCookie) {
        _newCookie.emplace();
    }
    if (_newCookie->has(name)) {
        _newCookie->erase(name);
    }
    (*_newCookie)[name] = value;
    auto &morsel = _newCookie->at(name);
    if (!domain.empty()) {
        morsel["domain"] = domain;
    }
    DateTime expiresTime(expires);
    if (expiresDays && expiresTime.is_not_a_date_time()) {
        expiresTime = boost::posix_time::second_clock::universal_time() + boost::gregorian::days(*expiresDays);
    }
    if (!expiresTime.is_not_a_date_time()) {
        morsel["expires"] = HTTPUtil::formatTimestamp(expiresTime);
    }
    if (!path.empty()) {
        morsel["path"] = path;
    }
    for (auto &kv: args) {
        if ((kv.first == "httponly" || kv.first == "secure") && kv.second.empty()) {
            continue;
        }
        morsel[kv.first] = kv.second;
    }
}

void RequestHandler::redirect(const std::string &url, bool permanent, boost::optional<int> status) {
    if (_headersWritten) {
        NET4CXX_THROW_EXCEPTION(Exception, "Cannot redirect after headers have been written");
    }
    if (!status) {
        status = permanent ? 301 : 302;
    } else {
        NET4CXX_ASSERT(300 <= *status && *status <= 399);
    }
    setStatus(*status);
//    std::string location = UrlParse::urlJoin(_request->getURI(), url);
    setHeader("Location", url);
    finish();
}

void RequestHandler::flush(bool includeFooters, FlushCallbackType callback) {
    auto connection = getConnection();
    std::string chunk = boost::join(_writeBuffer, "");
    _writeBuffer.clear();
    if (!_headersWritten) {
        _headersWritten = true;
        for (auto &transform: _transforms) {
            transform->transformFirstChunk(_statusCode, _headers, chunk, includeFooters);
        }
        if (_request->getMethod() == "HEAD") {
            chunk.clear();
        }
        if (_newCookie) {
            _newCookie->getAll([this](const std::string &key, const Morsel &cookie) {
                addHeader("Set-Cookie", cookie.outputString());
            });
        }
        auto startLine = ResponseStartLine("", _statusCode, _reason);
        connection->writeHeaders(std::move(startLine), _headers, chunk, std::move(callback));
    } else {
        for (auto &transform: _transforms) {
            transform->transformChunk(chunk, includeFooters);
        }
        if (_request->getMethod() != "HEAD") {
            connection->writeChunk(chunk, std::move(callback));
        }
    }
}

void RequestHandler::finish() {
    NET4CXX_ASSERT(!_finished);
    if (!_headersWritten) {
        const std::string &method = _request->getMethod();
        if (_statusCode == 200 && (method == "GET" || method == "HEAD") && !_headers.has("Etag")) {
            setEtagHeader();
            if (checkEtagHeader()) {
                _writeBuffer.clear();
                setStatus(304);
            }
        }
        if (_statusCode == 204 || _statusCode == 304 || (_statusCode >= 100 && _statusCode < 200)) {
            NET4CXX_ASSERT_THROW(_writeBuffer.empty(), "Cannot send body with %d", _statusCode);
            clearHeadersFor304();
        } else if (!_headers.has("Content-Length")) {
            size_t contentLength = std::accumulate(_writeBuffer.begin(), _writeBuffer.end(), (size_t)0,
                                                   [](size_t lhs, const std::string &rhs) {
                                                       return lhs + rhs.size();
                                                   });
            setHeader("Content-Length", contentLength);
        }
    }
    auto connection = _request->getConnection();
    if (connection) {
        connection->setCloseCallback(nullptr);
    }
    flush(true);
    _request->finish();
    log();
    _finished = true;
    onFinish();
}

void RequestHandler::sendError(int statusCode, const std::exception_ptr &error, std::string reason) {
    if (_headersWritten) {
        NET4CXX_LOG_ERROR(gGenLog, "Cannot send error response after headers written");
        if (!_finished) {
            try {
                finish();
            } catch (std::exception &e) {
                NET4CXX_LOG_ERROR(gGenLog, "Failed to flush partial response: %s", e.what());
            }
        }
        return;
    }
    clear();
    if (error) {
        try {
            std::rethrow_exception(error);
        } catch (HTTPError &e) {
            reason = e.getReason();
        } catch (...) {

        }
    }
    setStatus(statusCode, reason);
    try {
        writeError(statusCode, error);
    } catch (std::exception &e) {
        NET4CXX_LOG_ERROR(gAppLog, "Uncaught exception in writeError: %s", e.what());
    } catch (...) {
        NET4CXX_LOG_ERROR(gAppLog, "Unknown exception in writeError");
    }
    if (!_finished) {
        finish();
    }
}

void RequestHandler::writeError(int statusCode, const std::exception_ptr &error) {
    auto &settings = getSettings();
    auto iter = settings.find("serveTraceback");
    if (error && iter != settings.end() && boost::any_cast<bool>(iter->second)) {
        setHeader("Content-Type", "text/plain");
        try {
            std::rethrow_exception(error);
        } catch (std::exception &e) {
            write(e.what());
        } catch (...) {
            write("Unknown exception");
        }
        finish();
    } else {
        finish(StrUtil::format("<html><title>%d: %s</title><body>%d: %s</body></html>", statusCode, _reason, statusCode,
                               _reason));
    }
}

void RequestHandler::requireSetting(const std::string &name, const std::string &feature) {
    const auto &settings = _application->getSettings();
    if (settings.find("name") == settings.end()) {
        NET4CXX_THROW_EXCEPTION(Exception, "You must define the '%s' setting in your application to use %s", name,
                                feature);
    }
}

std::string RequestHandler::computeEtag() const {
    SHA1Object hasher;
    for (auto &part: _writeBuffer) {
        hasher.update(part);
    }
    std::string etag = "\"" + hasher.hex() + "\"";
    return etag;
}

bool RequestHandler::checkEtagHeader() const {
    auto computedEtag = _headers.get("Etag", "");
    if (computedEtag.empty()) {
        return false;
    }
    std::string inm = _request->getHTTPHeaders()->get("If-None-Match", "");
    if (inm.empty()) {
        return false;
    }
    std::string etag;
    const boost::regex pat(R"(\*|(?:W/)?"[^"]*")");
    boost::smatch what;
    bool match = false;
    auto start = inm.cbegin(), end = inm.cend();
    while (boost::regex_search(start, end, what, pat)) {
        etag = what[0].str();
        if (start == inm.cbegin()) {
            if (etag == "*") {
                match = true;
                break;
            }
            if (boost::starts_with(computedEtag, "W/")) {
                computedEtag.erase(0, 2);
            }
        }
        if (boost::starts_with(etag, "W/")) {
            etag.erase(0, 2);
        }
        if (etag == computedEtag) {
            match = true;
            break;
        }
        start = what[0].second;
    }

    return match;
}

const StringSet RequestHandler::SUPPORTED_METHODS = {
        "GET", "HEAD", "POST", "DELETE", "PATCH", "PUT", "OPTIONS"
};

std::string RequestHandler::getArgument(const std::string &name, const QueryArgListMap &source,
                                        const char *defaultValue, bool strip) const {
    auto iter = source.find(name);
    if (iter == source.end()) {
        if (defaultValue == nullptr) {
            NET4CXX_THROW_EXCEPTION(MissingArgumentError, "") << errinfo_http_code(400) << errinfo_arg_name(name);
        }
        return std::string(defaultValue);
    }
    std::string value = iter->second.back();
    boost::regex_replace(value, _removeControlCharsRegex, " ");
    if (strip) {
        boost::trim(value);
    }
    return value;
}

StringVector RequestHandler::getArguments(const std::string &name, const QueryArgListMap &source, bool strip) const {
    StringVector values;
    auto iter = source.find(name);
    if (iter == source.end()) {
        return values;
    }
    values = iter->second;
    for (auto &value: values) {
        boost::regex_replace(value, _removeControlCharsRegex, " ");
        if (strip) {
            boost::trim(value);
        }
    }
    return values;
}

void RequestHandler::execute(TransformsType transforms, const StringVector &args) {
    _transforms = std::move(transforms);
    std::exception_ptr error;
    try {
        const std::string &method = _request->getMethod();
        if (SUPPORTED_METHODS.find(method) == SUPPORTED_METHODS.end()) {
            NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
        }
        _pathArgs = args;
        auto result = prepare();
        if (result) {
            result->addCallbacks([this, self=shared_from_this()](DeferredValue value) {
                whenComplete();
                return value;
            }, [this, self=shared_from_this()](DeferredValue value) {
                try {
                    handleRequestException(value.asError());
                } catch (std::exception &e) {
                    NET4CXX_LOG_ERROR(gAppLog, "Exception in exception handler: %s", e.what());
                }
                return value;
            });
        } else {
            whenComplete();
        }
    } catch (...) {
        error = std::current_exception();
    }
    if (error) {
        try {
            handleRequestException(error);
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gAppLog, "Exception in exception handler: %s", e.what());
        }
    }
}

void RequestHandler::whenComplete() {
    std::exception_ptr error;
    try {
        executeMethod();
    } catch (...) {
        error = std::current_exception();
    }
    if (error) {
        try {
            handleRequestException(error);
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gAppLog, "Exception in exception handler: %s", e.what());
        }
    }
}

void RequestHandler::executeMethod() {
    if (!_finished) {
        DeferredPtr result;
        const std::string &method = _request->getMethod();
        if (method == "HEAD") {
            result = onHead(_pathArgs);
        } else if (method == "GET") {
            result = onGet(_pathArgs);
        } else if (method == "POST") {
            result = onPost(_pathArgs);
        } else if (method == "DELETE") {
            result = onDelete(_pathArgs);
        } else if (method == "PATCH") {
            result = onPatch(_pathArgs);
        } else if (method == "PUT") {
            result = onPut(_pathArgs);
        } else {
            assert(method == "OPTIONS");
            result = onOptions(_pathArgs);
        }
        if (result) {
            result->addCallback([this, self=shared_from_this()](DeferredValue value) {
                executeFinish();
                return value;
            })->addErrback([this, self=shared_from_this()](DeferredValue value) {
                try {
                    handleRequestException(value.asError());
                } catch (std::exception &e) {
                    NET4CXX_LOG_ERROR(gAppLog, "Exception in exception handler: %s", e.what());
                }
                return value;
            });
        } else {
            executeFinish();
        }
    }
}

void RequestHandler::log() {
    _application->logRequest(shared_from_this());
}

void RequestHandler::handleRequestException(const std::exception_ptr &error) {
    try {
        logException(error);
    } catch (std::exception &e) {
        NET4CXX_LOG_ERROR(gAppLog, "Error in exception logger: %s", e.what());
    }
    if (_finished) {
        return;
    }
    try {
        std::rethrow_exception(error);
    } catch (HTTPError &e) {
        int statusCode = e.getCode();
        if (HTTP_STATUS_CODES.find(statusCode) == HTTP_STATUS_CODES.end() && !e.hasReason()) {
            NET4CXX_LOG_ERROR(gGenLog, "Bad HTTP status code: %d", statusCode);
            sendError(500, error);
        } else {
            sendError(statusCode, error);
        }
    } catch (...) {
        sendError(500, error);
    }
}

void RequestHandler::logException(const std::exception_ptr &error) {
    try {
        std::rethrow_exception(error);
    } catch (HTTPError &e) {
        std::string summary = requestSummary();
        int statusCode = e.getCode();
        NET4CXX_LOG_WARN(gGenLog, "%d %s: %s", statusCode, summary, e.what());
    } catch (std::exception &e) {
        std::string summary = requestSummary();
        NET4CXX_LOG_ERROR(gAppLog, "Uncaught exception %s\n%s\n%s", e.what(), summary,
                          boost::lexical_cast<std::string>(*_request));
    } catch (...) {
        std::string summary = requestSummary();
        NET4CXX_LOG_ERROR(gAppLog, "Unknown exception\n%s\n%s", summary, boost::lexical_cast<std::string>(*_request));
    }
}


void ErrorHandler::initialize(const ArgsType &args) {
    setStatus(boost::any_cast<int>(args.at("statusCode")));
}

DeferredPtr ErrorHandler::prepare() {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(_statusCode);
    return nullptr;
}


void RedirectHandler::initialize(const ArgsType &args) {
    _url = boost::any_cast<std::string>(args.at("url"));
    auto iter = args.find("permanent");
    if (iter != args.end()) {
        _permanent = boost::any_cast<bool>(iter->second);
    }
}

DeferredPtr RedirectHandler::onGet(const StringVector &args) {
    if (args.empty()) {
        redirect(_url, _permanent);
    } else {
        boost::format fmt(_url);
        for (auto &arg: args) {
            fmt % arg;
        }
        redirect(fmt.str(), _permanent);
    }
    return nullptr;
}


void FallbackHandler::initialize(const ArgsType &args) {
    _fallback = boost::any_cast<FallbackType>(args.at("fallback"));
}

DeferredPtr FallbackHandler::prepare() {
    _fallback(_request);
    _finished = true;
    return nullptr;
}


const StringSet GZipContentEncoding::CONTENT_TYPES = {
        "application/javascript",
        "application/x-javascript",
        "application/xml",
        "application/atom+xml",
        "application/json",
        "application/xhtml+xml",
        "image/svg+xml"
};

constexpr int GZipContentEncoding::GZIP_LEVEL;

constexpr int GZipContentEncoding::MIN_LENGTH;

GZipContentEncoding::GZipContentEncoding(const std::shared_ptr<HTTPServerRequest> &request) {
    auto headers = request->getHTTPHeaders();
    std::string acceptEncoding = headers->get("Accept-Encoding");
    _gzipping = acceptEncoding.find("gzip") != std::string::npos;
}

void GZipContentEncoding::transformFirstChunk(int &statusCode, HTTPHeaders &headers, std::string &chunk,
                                              bool finishing) {
    if (headers.has("Vary")) {
        headers["Vary"] = headers.at("Vary") + ", Accept-Encoding";
    } else {
        headers["Vary"] = "Accept-Encoding";
    }
    if (_gzipping) {
        std::string ctype = headers.get("Content-Type", "");
        auto pos = ctype.find(';');
        if (pos != std::string::npos) {
            ctype = ctype.substr(0, pos);
        }
        _gzipping = compressibleType(ctype) &&
                    (!finishing || chunk.size() >= MIN_LENGTH) &&
                    !headers.has("Content-Encoding");
    }
    if (_gzipping) {
        headers["Content-Encoding"] = "gzip";
        _gzipValue = std::make_shared<std::stringstream>();
        _gzipFile.initWithOutputStream(_gzipValue, GZIP_LEVEL);
        transformChunk(chunk, finishing);
        if (headers.has("Content-Length")) {
            if (finishing) {
                headers["Content-Length"] = std::to_string(chunk.size());
            } else {
                headers.erase("Content-Length");
            }
        }
    }
}

void GZipContentEncoding::transformChunk(std::string &chunk, bool finishing) {
    if (_gzipping) {
        _gzipFile.write(chunk);
        if (finishing) {
            _gzipFile.close();
        } else {
            _gzipFile.flush();
        }
        auto length = _gzipValue->tellp() - _gzipValue->tellg();
        chunk.resize((size_t)length);
        _gzipValue->read((char *)chunk.data(), length);
    }
}

bool GZipContentEncoding::compressibleType(const std::string &ctype) {
    return boost::starts_with(ctype, "text/") || CONTENT_TYPES.find(ctype) != CONTENT_TYPES.end();
}


void RequestDispatcher::execute() {
    RequestHandler::TransformsType transforms;
    for (auto &transform: _application->getTransforms()) {
        transforms.emplace_back(transform->create(_request));
    }
    _handler->execute(std::move(transforms), _pathArgs);
}

void RequestDispatcher::findHandler() {
    auto handlers = _application->getHostHandlers(_request);
    if (handlers.empty()) {
        RequestHandler::ArgsType handlerArgs = {
                {"url", _request->getProtocol() + "://" + _application->getDefaultHost() + "/"}
        };
        _handler = RequestHandlerFactory<RedirectHandler>().create(_application, _request, handlerArgs);
        return;
    }
    const std::string &requestPath = _request->getPath();
    boost::smatch match;
    for (auto &spec: handlers) {
        if (boost::regex_match(requestPath, match, spec->getRegex())) {
            _handler = spec->getHandlerFactory()->create(_application, _request, spec->getArgs());
            for (size_t i = 1; i < match.size(); ++i) {
                _pathArgs.emplace_back(match[i].str());
            }
            for (auto &s: _pathArgs) {
                s = UrlParse::unquote(s);
            }
            return;
        }
    }
    auto iter = _application->getSettings().find("defaultHandlerFactory");
    if (iter != _application->getSettings().end()) {
        auto handlerFactory = boost::any_cast<RequestHandlerFactoryPtr>(iter->second);
        auto argIter = _application->getSettings().find("defaultHandlerArgs");
        if (argIter != _application->getSettings().end()) {
            auto &handlerArgs = boost::any_cast<RequestHandler::ArgsType&>(argIter->second);
            _handler = handlerFactory->create(_application, _request, handlerArgs);
        } else {
            RequestHandler::ArgsType handlerArgs;
            _handler = handlerFactory->create(_application, _request, handlerArgs);
        }
    } else {
        RequestHandler::ArgsType handlerArgs = {
                {"statusCode", 404}
        };
        _handler = RequestHandlerFactory<ErrorHandler>().create(_application, _request, handlerArgs);
    }
}


WebApp::WebApp(HandlersType handlers, std::string defaultHost, TransformsType transforms, SettingsType settings)
        : _defaultHost(std::move(defaultHost))
        , _settings(std::move(settings)) {
    if (transforms.empty()) {
        if (_settings.find("compressResponse") != _settings.end() &&
            boost::any_cast<bool>(_settings["compressResponse"])) {
            addTransform<GZipContentEncoding>();
        }
    } else {
        _transforms = std::move(transforms);
    }
    if (!handlers.empty()) {
        addHandlers(".*$", std::move(handlers));
    }
}

ProtocolPtr WebApp::buildProtocol(const Address &address) {
    return std::make_shared<HTTPConnection>(_maxBufferSize);
}

void WebApp::addHandlers(std::string hostPattern, HandlersType hostHandlers) {
    if (!boost::ends_with(hostPattern, "$")) {
        hostPattern.push_back('$');
    }
    if (!_handlers.empty() && _handlers.back().first.str() == ".*$") {
        auto iter = _handlers.end();
        std::advance(iter, -1);
        _handlers.insert(iter, std::make_pair(HostPatternType{hostPattern}, hostHandlers));
    } else {
        _handlers.emplace_back(std::make_pair(HostPatternType{hostPattern}, hostHandlers));
    }
    for (auto &spec: hostHandlers) {
        if (!spec->getName().empty()) {
            if (_namedHandlers.find(spec->getName()) != _namedHandlers.end()) {
                NET4CXX_LOG_WARN(gAppLog, "Multiple handlers named %s; replacing previous value", spec->getName());
            }
            _namedHandlers[spec->getName()] = spec;
        }
    }
}

void WebApp::logRequest(std::shared_ptr<const RequestHandler> handler) const {
    auto iter = _settings.find("logFunction");
    if (iter != _settings.end()) {
        const auto &logFunction = boost::any_cast<const LogFunctionType&>(iter->second);
        logFunction(std::move(handler));
        return;
    }
    int statusCode = handler->getStatus();
    std::string summary = handler->requestSummary();
    double requestTime = 1000.0 * handler->_request->requestTime();
    std::string logInfo = StrUtil::format("%d %s %.2fms", statusCode, summary, requestTime);
    if (statusCode < 400) {
        NET4CXX_LOG_INFO(gAccessLog, logInfo.c_str());
    } else if (statusCode < 500) {
        NET4CXX_LOG_WARN(gAccessLog, logInfo.c_str());
    } else {
        NET4CXX_LOG_ERROR(gAccessLog, logInfo.c_str());
    }
}

std::vector<UrlSpecPtr> WebApp::getHostHandlers(const std::shared_ptr<const HTTPServerRequest> &request) const {
    std::string host;
    std::tie(host, std::ignore) = HTTPUtil::splitHostAndPort(boost::to_lower_copy(request->getHost()));
    std::vector<UrlSpecPtr> matches;
    for (auto &handler: _handlers) {
        if (boost::regex_match(host, handler.first)) {
            matches.insert(matches.end(), handler.second.begin(), handler.second.end());
        }
    }
    if (matches.empty() && !request->getHTTPHeaders()->has("X-Real-Ip")) {
        for (auto &handler: _handlers) {
            if (boost::regex_match(_defaultHost, handler.first)) {
                matches.insert(matches.end(), handler.second.begin(), handler.second.end());
            }
        }
    }
    return matches;
}

NS_END