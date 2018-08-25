//
// Created by yuwenyong.vincent on 2018/8/11.
//

#include "net4cxx/plugins/web/web.h"
#include "net4cxx/common/crypto/hashlib.h"


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

void RequestHandler::start(ArgsType &args) {
    _request->getConnection()->setCloseCallback([this, self=shared_from_this()](){
        onConnectionClose();
    });
    initialize(args);
}

void RequestHandler::initialize(ArgsType &args) {

}

const RequestHandler::SettingsType& RequestHandler::getSettings() const {
    return _application->getSettings();
}

void RequestHandler::onHead(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
}

void RequestHandler::onGet(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
}

void RequestHandler::onPost(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
}

void RequestHandler::onDelete(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
}

void RequestHandler::onPatch(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
}

void RequestHandler::onPut(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
}

void RequestHandler::onOptions(const StringVector &args) {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
}

void RequestHandler::prepare() {

}

void RequestHandler::onFinish() {

}

void RequestHandler::onConnectionClose() {

}

void RequestHandler::clear() {
    _headers = HTTPHeaders({
                                   {"Server", NET4CXX_VER },
                                   {"Content-Type", "text/html; charset=UTF-8"},
                                   {"Date", HTTPUtil::formatTimestamp(time(nullptr))},
                           });
    setDefaultHeaders();
    if (!_request->supportsHTTP11() && !_request->getConnection()->getNoKeepAlive()) {
        auto connHeader = _request->getHTTPHeaders()->get("Connection");
        if (!connHeader.empty() && boost::to_lower_copy(connHeader) == "keep-alive") {
//            setHeader("Connection", "Keep-Alive");
            _headers["Connection"] = "Keep-Alive";
        }
    }
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
//    const boost::regex patt(R"([\x00-\x20]+)");
//    std::string location = UrlParse::urlJoin(_request->getURI(), boost::regex_replace(url, patt, ""));
    std::string location = UrlParse::urlJoin(_request->getURI(), url);
    setHeader("Location", location);
    finish();
}

void RequestHandler::flush(bool includeFooters, FlushCallbackType callback) {
    ByteArray chunk = std::move(_writeBuffer);
    std::string headers;
    if (!_headersWritten) {
        _headersWritten = true;
        for (auto &transfrom: _transforms) {
            transfrom->transformFirstChunk(_statusCode, _headers, chunk, includeFooters);
        }
        headers = generateHeaders();
    } else {
        for (auto &transfrom: _transforms) {
            transfrom->transformChunk(chunk, includeFooters);
        }
    }
    if (_request->getMethod() == "HEAD") {
        if (!headers.empty()) {
            _request->write(headers, std::move(callback));
        }
        return;
    }
    if (!chunk.empty()) {
        headers.append((const char *)chunk.data(), chunk.size());
    }
    _request->write(headers, std::move(callback));
}

void RequestHandler::sendError(int statusCode, std::exception_ptr error) {
    if (_headersWritten) {
        NET4CXX_LOG_ERROR(gGenLog, "Cannot send error response after headers written");
        if (!_finished) {
            finish();
        }
        return;
    }
    clear();
    std::string reason;
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

void RequestHandler::writeError(int statusCode, std::exception_ptr error) {
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
    hasher.update(_writeBuffer);
    std::string etag = "\"" + hasher.hex() + "\"";
    return etag;
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

void RequestHandler::execute(TransformsType transforms, StringVector args) {
    _transforms = std::move(transforms);
    std::exception_ptr error;
    try {
        const std::string &method = _request->getMethod();
        if (SUPPORTED_METHODS.find(method) == SUPPORTED_METHODS.end()) {
            NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(405);
        }
        _pathArgs = std::move(args);
        prepare();
        if (_autoFinish) {
            executeMethod();
        }
    } catch (...) {
        error = std::current_exception();
    }
    if (error) {
        handleRequestException(error);
    }
}

//void RequestHandler::whenComplete() {
//    std::exception_ptr error;
//    try {
//        executeMethod();
//    } catch (...) {
//        error = std::current_exception();
//    }
//    if (error) {
//        handleRequestException(error);
//    }
//}

void RequestHandler::executeMethod() {
    if (!_finished) {
        const std::string &method = _request->getMethod();
        if (method == "HEAD") {
            onHead(_pathArgs);
        } else if (method == "GET") {
            onGet(_pathArgs);
        } else if (method == "POST") {
            onPost(_pathArgs);
        } else if (method == "DELETE") {
            onDelete(_pathArgs);
        } else if (method == "PATCH") {
            onPatch(_pathArgs);
        } else if (method == "PUT") {
            onPut(_pathArgs);
        } else {
            assert(method == "OPTIONS");
            onOptions(_pathArgs);
        }
        executeFinish();
    }
}

std::string RequestHandler::generateHeaders() const {
    StringVector lines;
    lines.emplace_back(_request->getVersion() + " " + std::to_string(_statusCode) + " " + _reason);
    _headers.getAll([&lines](const std::string &name, const std::string &value) {
        lines.emplace_back(name + ": " + value);
    });
    if (_newCookie) {
        _newCookie->getAll([&lines](const std::string &key, const Morsel &cookie) {
            lines.emplace_back("Set-Cookie: " + cookie.outputString());
        });
    }
    return boost::join(lines, "\r\n") + "\r\n\r\n";
}

void RequestHandler::log() {
    _application->logRequest(shared_from_this());
}

void RequestHandler::handleRequestException(std::exception_ptr error) {
    logException(error);
    if (_finished) {
        return;
    }
    try {
        std::rethrow_exception(error);
    } catch (HTTPError &e) {
        std::string summary = requestSummary();
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

void RequestHandler::logException(std::exception_ptr error) {
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


void ErrorHandler::initialize(ArgsType &args) {
    setStatus(boost::any_cast<int>(args.at("statusCode")));
}

void ErrorHandler::prepare() {
    NET4CXX_THROW_EXCEPTION(HTTPError, "") << errinfo_http_code(_statusCode);
}


void RedirectHandler::initialize(ArgsType &args) {
    _url = boost::any_cast<std::string>(args.at("url"));
    auto iter = args.find("permanent");
    if (iter != args.end()) {
        _permanent = boost::any_cast<bool>(iter->second);
    }
}

void RedirectHandler::onGet(const StringVector &args) {
    redirect(_url, _permanent);
}


void FallbackHandler::initialize(ArgsType &args) {
    _fallback = boost::any_cast<FallbackType>(args.at("fallback"));
}

void FallbackHandler::prepare() {
    _fallback(_request);
    _finished = true;
}


UrlSpec::UrlSpec(std::string pattern, RequestHandlerFactoryPtr handlerFactory, std::string name)
        : UrlSpec(std::move(pattern), std::move(handlerFactory), {}, std::move(name)) {

}

UrlSpec::UrlSpec(std::string pattern, RequestHandlerFactoryPtr handlerFactory, ArgsType args, std::string name)
        : _pattern(std::move(pattern))
        , _handlerFactory(std::move(handlerFactory))
        , _args(std::move(args))
        , _name(std::move(name)) {
    if (!boost::ends_with(_pattern, "$")) {
        _pattern.push_back('$');
    }
    _regex = _pattern;
    std::tie(_path, _groupCount) = findGroups();
}

std::tuple<std::string, int> UrlSpec::findGroups() {
    auto beg = _pattern.begin(), end = _pattern.end();
    std::string pattern;
    if (boost::starts_with(_pattern, "^")) {
        std::advance(beg, 1);
    }
    if (boost::ends_with(_pattern, "$")) {
        std::advance(end, -1);
    }
    pattern.assign(beg, end);
    if (_regex.mark_count() != StrUtil::count(pattern, '(')) {
        return std::make_tuple("", -1);
    }
    StringVector pieces, fragments;
    std::string::size_type parenLoc;
    fragments = StrUtil::split(pattern, '(');
    for (auto &fragment: fragments) {
        parenLoc = fragment.find(')');
        if (parenLoc != std::string::npos) {
            pieces.push_back("%s" + fragment.substr(parenLoc + 1));
        } else {
            pieces.push_back(std::move(fragment));
        }
    }
    return std::make_tuple(boost::join(pieces, ""), (int)_regex.mark_count());
}


const StringSet GZipContentEncoding::CONTENT_TYPES = {
        "text/plain",
        "text/html",
        "text/css",
        "text/xml",
        "application/javascript",
        "application/x-javascript",
        "application/xml",
        "application/atom+xml",
        "text/javascript",
        "application/json",
        "application/xhtml+xml"
};

constexpr int GZipContentEncoding::MIN_LENGTH;

GZipContentEncoding::GZipContentEncoding(HTTPServerRequestPtr request) {
    if (request->supportsHTTP11()) {
        auto headers = request->getHTTPHeaders();
        std::string acceptEncoding = headers->get("Accept-Encoding");
        _gzipping = acceptEncoding.find("gzip") != std::string::npos;
    } else {
        _gzipping = false;
    }
}

void GZipContentEncoding::transformFirstChunk(int &statusCode, HTTPHeaders &headers, ByteArray &chunk, bool finishing) {
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
        _gzipping = CONTENT_TYPES.find(ctype) != CONTENT_TYPES.end()
                    && (!finishing || chunk.size() >= MIN_LENGTH)
                    && (finishing || !headers.has("Content-Length"))
                    && !headers.has("Content-Encoding");
    }
    if (_gzipping) {
        headers["Content-Encoding"] = "gzip";
        _gzipValue = std::make_shared<std::stringstream>();
        _gzipFile.initWithOutputStream(_gzipValue);
        transformChunk(chunk, finishing);
        if (headers.has("Content-Length")) {
            headers["Content-Length"] = std::to_string(chunk.size());
        }
    }
}

void GZipContentEncoding::transformChunk(ByteArray &chunk, bool finishing) {
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



void ChunkedTransferEncoding::transformFirstChunk(int &statusCode, HTTPHeaders &headers, ByteArray &chunk,
                                                  bool finishing) {
    if (_chunking && statusCode != 304) {
        if (headers.has("Content-Length") || headers.has("Transfer-Encoding")) {
            _chunking = false;
        } else {
            headers["Transfer-Encoding"] = "chunked";
            transformChunk(chunk, finishing);
        }
    }
}

void ChunkedTransferEncoding::transformChunk(ByteArray &chunk, bool finishing) {
    if (_chunking) {
        std::string block;
        if (!chunk.empty()) {
            block = StrUtil::format("%x\r\n", (int)chunk.size());
            chunk.insert(chunk.begin(), (const Byte *)block.data(), (const Byte *)block.data() + block.size());
            block = "\r\n";
            chunk.insert(chunk.end(), (const Byte *)block.data(), (const Byte *)block.data() + block.size());
        }
        if (finishing) {
            block = "0\r\n\r\n";
            chunk.insert(chunk.end(), (const Byte *)block.data(), (const Byte *)block.data() + block.size());
        }
    }
}


WebApp::WebApp(HandlersType handlers, std::string defaultHost, TransformsType transforms, SettingsType settings)
        : _defaultHost(std::move(defaultHost))
        , _settings(std::move(settings)) {
    if (transforms.empty()) {
        if (_settings.find("gzip") != _settings.end() && boost::any_cast<bool>(_settings["gzip"])) {
            addTransform<GZipContentEncoding>();
        }
        addTransform<ChunkedTransferEncoding>();
    } else {
        _transforms = std::move(transforms);
    }
    if (!handlers.empty()) {
        addHandlers(".*$", std::move(handlers));
    }
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
    for (auto spec: hostHandlers) {
        if (!spec->getName().empty()) {
            if (_namedHandlers.find(spec->getName()) != _namedHandlers.end()) {
                NET4CXX_LOG_WARN(gAppLog, "Multiple handlers named %s; replacing previous value", spec->getName());
            }
            _namedHandlers[spec->getName()] = spec;
        }
    }
}


void WebApp::operator()(HTTPServerRequestPtr request) {
    RequestHandler::TransformsType transforms;
    for (auto &transform: _transforms) {
        transforms.emplace_back(transform->create(request));
    }
    RequestHandlerPtr handler;
    StringVector args;
    auto handlers = getHostHandlers(request);
    if (handlers.empty()) {
        RequestHandler::ArgsType handlerArgs = {
                {"url", "http://" + _defaultHost + "/"}
        };
        handler = RequestHandlerFactory<RedirectHandler>().create(shared_from_this(), std::move(request), handlerArgs);
    } else {
        const std::string &requestPath = request->getPath();
        boost::smatch match;
        for (auto spec: handlers) {
            if (boost::regex_match(requestPath, match, spec->getRegex())) {
                handler = spec->getHandlerFactory()->create(shared_from_this(), std::move(request), spec->args());
                for (size_t i = 1; i < match.size(); ++i) {
                    args.emplace_back(match[i].str());
                }
                for (auto &s: args) {
                    s = UrlParse::unquote(s);
                }
                break;
            }
        }
        if (!handler) {
            auto iter = _settings.find("defaultHandlerFactory");
            if (iter != _settings.end()) {
                auto handlerFactory = boost::any_cast<RequestHandlerFactoryPtr>(iter->second);
                auto argIter = _settings.find("defaultHandlerArgs");
                if (argIter != _settings.end()) {
                    auto &handlerArgs = boost::any_cast<RequestHandler::ArgsType&>(argIter->second);
                    handler = handlerFactory->create(shared_from_this(), std::move(request), handlerArgs);
                } else {
                    RequestHandler::ArgsType handlerArgs;
                    handler = handlerFactory->create(shared_from_this(), std::move(request), handlerArgs);
                }
            } else {
                RequestHandler::ArgsType handlerArgs = {
                        {"statusCode", 404}
                };
                handler = RequestHandlerFactory<ErrorHandler>().create(shared_from_this(), std::move(request),
                                                                       handlerArgs);
            }
        }
    }
    handler->execute(std::move(transforms), std::move(args));
}

void WebApp::logRequest(RequestHandlerConstPtr handler) const {
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

std::vector<UrlSpecPtr> WebApp::getHostHandlers(HTTPServerRequestConstPtr request) {
    std::string host = request->getHost();
    boost::to_lower(host);
    auto pos = host.find(':');
    if (pos != std::string::npos) {
        host = host.substr(0, pos);
    }
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