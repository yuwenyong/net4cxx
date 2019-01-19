//
// Created by yuwenyong.vincent on 2018/7/22.
//

#include "net4cxx/plugins/web/httputil.h"
#include "net4cxx/common/email/utils.h"
#include "net4cxx/common/httputils/cookie.h"
#include "net4cxx/shared/global/loggers.h"


NS_BEGIN

const boost::regex HTTPHeaders::_normalizedHeader("^[A-Z0-9][a-z0-9]*(-[A-Z0-9][a-z0-9]*)*$");

void HTTPHeaders::add(const std::string &name, const std::string &value) {
    std::string normName = normalizeName(name);
    _lastKey = normName;
    if (has(normName)) {
        _items[normName] += ',' + value;
        _asList[normName].emplace_back(value);
    } else {
        (*this)[normName] = value;
    }
}

StringVector HTTPHeaders::getList(const std::string &name) const {
    std::string normName = normalizeName(name);
    auto iter = _asList.find(normName);
    if (iter != _asList.end()) {
        return iter->second;
    } else {
        return {};
    }
}

void HTTPHeaders::parseLine(const std::string &line) {
    NET4CXX_ASSERT(!line.empty());
    if (std::isspace(line[0])) {
        if (_lastKey.empty()) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "first header line cannot start with whitespace");
        }
        std::string newPart = " " + boost::trim_left_copy(line);
        auto &asList = _asList.at(_lastKey);
        NET4CXX_ASSERT(!asList.empty());
        asList.back() += newPart;
        _items.at(_lastKey) += newPart;
    } else {
        size_t pos = line.find(':');
        if (pos == 0 || pos == std::string::npos) {
            NET4CXX_THROW_EXCEPTION(HTTPInputError, "no colon in header line");
        }
        std::string name = line.substr(0, pos);
        std::string value = line.substr(pos + 1, std::string::npos);
        boost::trim(value);
        add(name, value);
    }
}

void HTTPHeaders::erase(const std::string &name) {
    std::string normName = HTTPHeaders::normalizeName(name);
    if (!has(normName)) {
        NET4CXX_THROW_EXCEPTION(KeyError, normName);
    }
    _items.erase(normName);
    _asList.erase(normName);
}

std::string HTTPHeaders::get(const std::string &name, const std::string &defaultValue) const {
    std::string normName = HTTPHeaders::normalizeName(name);
    auto iter = _items.find(normName);
    if (iter != _items.end()) {
        return iter->second;
    } else {
        return defaultValue;
    }
}

void HTTPHeaders::parseLines(const std::string &headers) {
    StringVector lines = StrUtil::splitLines(headers);
    for (auto &line: lines) {
        if (!line.empty()) {
            parseLine(line);
        }
    }
}

std::string HTTPHeaders::normalizeName(const std::string &name) {
//    if (boost::regex_match(name, HTTPHeaders::_normalizedHeader)) {
//        return name;
//    }
    StringVector nameParts = StrUtil::split(name, '-');
    for (auto &namePart: nameParts) {
        StrUtil::capitalize(namePart);
    }
    return boost::join(nameParts, "-");
}


std::ostream& operator<<(std::ostream &os, const HTTPHeaders &headers) {
    StringVector lines;
    headers.getAll([&lines](const std::string &name, const std::string &value) {
        lines.emplace_back(StrUtil::format("%s: %s\n", name, value));
    });
    os << boost::join(lines, "");
    return os;
}


std::string HTTPUtil::urlConcat(std::string url, const QueryArgMap &args) {
    if (args.empty()) {
        return url;
    }
    auto parsedUrl = UrlParse::urlParse(std::move(url));
    auto parsedQuery = UrlParse::parseQSL(parsedUrl.getQuery(), true);
    for (auto &kv: args) {
        parsedQuery.emplace_back(kv.first, kv.second);
    }
    auto finalQuery = UrlParse::urlEncode(parsedQuery);
    url = UrlParse::urlUnparse({
                                       parsedUrl.getScheme(),
                                       parsedUrl.getNetloc(),
                                       parsedUrl.getPath(),
                                       parsedUrl.getParams(),
                                       finalQuery,
                                       parsedUrl.getFragment()
                               });
    return url;
}

std::string HTTPUtil::urlConcat(std::string url, const QueryArgList &args) {
    if (args.empty()) {
        return url;
    }
    auto parsedUrl = UrlParse::urlParse(std::move(url));
    auto parsedQuery = UrlParse::parseQSL(parsedUrl.getQuery(), true);
    parsedQuery.insert(parsedQuery.end(), args.begin(), args.end());
    auto finalQuery = UrlParse::urlEncode(parsedQuery);
    url = UrlParse::urlUnparse({
                                       parsedUrl.getScheme(),
                                       parsedUrl.getNetloc(),
                                       parsedUrl.getPath(),
                                       parsedUrl.getParams(),
                                       finalQuery,
                                       parsedUrl.getFragment()
                               });
    return url;
}

void HTTPUtil::parseBodyArguments(const std::string &contentType, const std::string &body, QueryArgListMap &arguments,
                                  HTTPFileListMap &files, const HTTPHeaders *headers) {
    if (headers && headers->has("Content-Encoding")) {
        NET4CXX_LOG_WARN(gGenLog, "Unsupported Content-Encoding: %s", headers->at("Content-Encoding"));
        return;
    }
    if (boost::starts_with(contentType, "application/x-www-form-urlencoded")) {
        QueryArgListMap uriArguments;
        try {
            uriArguments = UrlParse::parseQS(body, true);
        } catch (std::exception &e) {
            NET4CXX_LOG_WARN(gGenLog, "Invalid x-www-form-urlencoded body: %s", e.what());
        }
        for (auto &nv: uriArguments) {
            if (!nv.second.empty()) {
                for (auto &value: nv.second) {
                    arguments[nv.first].push_back(std::move(value));
                }
            }
        }
    } else if (boost::starts_with(contentType, "multipart/form-data")) {
        StringVector fields = StrUtil::split(contentType, ';');
        bool found = false;
        std::string k, sep, v;
        for (auto &field: fields) {
            boost::trim(field);
            std::tie(k, sep, v) = StrUtil::partition(field, "=");
            if (k == "boundary" && !v.empty()) {
                HTTPUtil::parseMultipartFormData(std::move(v), body, arguments, files);
                found = true;
                break;
            }
        }
        if (!found) {
            NET4CXX_LOG_WARN(gGenLog, "Invalid multipart/form-data");
        }
    }
}

void HTTPUtil::parseMultipartFormData(std::string boundary, const std::string &data, QueryArgListMap &arguments,
                                      HTTPFileListMap &files) {
    if (boost::starts_with(boundary, "\"") && boost::ends_with(boundary, "\"")) {
        if (boundary.length() >= 2) {
            boundary = boundary.substr(1, boundary.length() - 2);
        } else {
            boundary.clear();
        }
    }
    size_t finalBoundaryIndex = data.rfind("--" + boundary + "--");
    if (finalBoundaryIndex == std::string::npos) {
        NET4CXX_LOG_WARN(gGenLog, "Invalid multipart/form-data: no final boundary");
        return;
    }
    StringVector parts = StrUtil::split(data.substr(0, finalBoundaryIndex), "--" + boundary + "\r\n");
    size_t eoh;
    HTTPHeaders headers;
    std::string dispHeader, disposition, name, value, ctype;
    StringMap dispParams;
    decltype(dispParams.begin()) nameIter, fileNameIter;
    for (auto &part: parts) {
        if (part.empty()) {
            continue;
        }
        eoh = part.find("\r\n\r\n");
        if (eoh == std::string::npos) {
            NET4CXX_LOG_WARN(gGenLog, "multipart/form-data missing headers");
            continue;
        }
        headers.clear();
        headers.parseLines(part.substr(0, eoh));
        dispHeader = headers.get("Content-Disposition");
        std::tie(disposition, dispParams) = parseHeader(dispHeader);
        if (disposition != "form-data" || !boost::ends_with(part, "\r\n")) {
            NET4CXX_LOG_WARN(gGenLog, "Invalid multipart/form-data");
            continue;
        }
        if (part.length() <= eoh + 6) {
            value.clear();
        } else {
            value = part.substr(eoh + 4, part.length() - eoh - 6);
        }
        nameIter = dispParams.find("name");
        if (nameIter == dispParams.end()) {
            NET4CXX_LOG_WARN(gGenLog, "multipart/form-data value missing name");
            continue;
        }
        name = nameIter->second;
        fileNameIter = dispParams.find("filename");
        if (fileNameIter != dispParams.end()) {
            ctype = headers.get("Content-Type", "application/unknown");
            files[name].emplace_back(HTTPFile(std::move(fileNameIter->second), std::move(ctype), std::move(value)));
        } else {
            arguments[name].emplace_back(std::move(value));
        }
    }
}

RequestStartLine HTTPUtil::parseRequestStartLine(const std::string &line) {
    StringVector requestLineComponents = StrUtil::split(line);
    if (requestLineComponents.size() != 3) {
        NET4CXX_THROW_EXCEPTION(HTTPInputError, "Malformed HTTP request line");
    }
    std::string method = std::move(requestLineComponents[0]);
    std::string path = std::move(requestLineComponents[1]);
    std::string version = std::move(requestLineComponents[2]);
    const boost::regex versionPat(R"(HTTP/1\.[0-9])");
    if (!boost::regex_match(version, versionPat)) {
        NET4CXX_THROW_EXCEPTION(HTTPInputError, "Malformed HTTP version in HTTP Request-Line: %s", version);
    }
    return RequestStartLine(std::move(method), std::move(path), std::move(version));
}

ResponseStartLine HTTPUtil::parseResponseStartLine(const std::string &line) {
    const boost::regex firstLinePattern("(HTTP/1.[0-9]) ([0-9]+) ([^\r]*).*");
    boost::smatch match;
    if (!boost::regex_match(line, match, firstLinePattern)) {
        NET4CXX_THROW_EXCEPTION(HTTPInputError, "Error parsing response start line");
    }
    return ResponseStartLine(match[1], std::stoi(match[2]), match[3]);
}

std::tuple<std::string, boost::optional<unsigned short>> HTTPUtil::splitHostAndPort(const std::string &netloc) {
    const boost::regex pat(R"(^(.+):(\d+)$)");
    boost::smatch match;
    std::string host;
    boost::optional<unsigned short> port;
    if (boost::regex_match(netloc, match, pat)) {
        host = match[1];
        port = (unsigned short) std::stoul(match[2]);
    } else {
        host = netloc;
    }
    return std::make_tuple(std::move(host), port);
}

QueryArgList HTTPUtil::QStoQSL(const QueryArgListMap &args) {
    QueryArgList res;
    for (auto &kv: args) {
        for (auto &v: kv.second) {
            res.emplace_back(kv.first, v);
        }
    }
    return res;
}

std::tuple<std::string, std::shared_ptr<HTTPHeaders>> HTTPUtil::parseHeaders(const char *data, size_t length) {
    boost::string_view dv{data, length};
    dv.remove_prefix(std::min(dv.find_first_not_of("\r\n"), dv.size()));
    auto eol = dv.find('\n');
    std::string startLine, rest;
    if (eol != boost::string_view::npos) {
        startLine.assign(dv.begin(), dv.begin() + eol);
        boost::trim_right_if(startLine, boost::is_any_of("\r"));
        rest.assign(dv.begin() + eol, dv.end());
    } else {
        startLine.assign(dv.begin(), dv.end());
    }
    auto headers = HTTPHeaders::parse(rest);
    return std::make_tuple(std::move(startLine), std::move(headers));
}

StringMap HTTPUtil::parseCookie(const std::string &cookie) {
    StringMap cookieDict;
    auto chunks = StrUtil::split(cookie, ';');
    std::string key, val;
    for (auto &chunk: chunks) {
        auto pos = chunk.find('=');
        if (pos != std::string::npos) {
            key.assign(chunk.begin(), chunk.begin() + (std::ptrdiff_t)pos);
            val.assign(chunk.begin() + (std::ptrdiff_t)pos + 1, chunk.end());
            boost::trim(key);
            boost::trim(val);
        } else {
            key = "";
            val = boost::trim_copy(chunk);
        }
        if (!key.empty() || !val.empty()) {
            cookieDict[std::move(key)] = CookieUtil::unquote(val);
        }
    }
    return cookieDict;
}

StringVector HTTPUtil::parseParam(std::string s) {
    StringVector parts;
    size_t end;
    while (!s.empty() && s[0] == ';') {
        s.erase(s.begin());
        end = s.find(';');
        while (end != std::string::npos
               && ((StrUtil::count(s, '"', 0, end) - StrUtil::count(s, "\\\"", 0, end)) % 2 != 0)) {
            end = s.find('"', end + 1);
        }
        if (end == std::string::npos) {
            end = s.size();
        }
        parts.emplace_back(boost::trim_copy(s.substr(0, end)));
        s = s.substr(end);
    }
    return parts;
}

std::tuple<std::string, StringMap> HTTPUtil::parseHeader(const std::string &line) {
    StringVector parts = parseParam(";" + line);
    std::string key = std::move(parts[0]);
    parts.erase(parts.begin());
    QueryArgList params{{"Dummy", "value"}};
    size_t i;
    std::string name, value;
    for (auto &p: parts) {
        i = p.find('=');
        if (i != std::string::npos) {
            name = boost::to_lower_copy(boost::trim_copy(p.substr(0, i)));
            value = boost::trim_copy(p.substr(i + 1));
            params.emplace_back(std::move(name), std::move(value));
        }
    }
    params = EMailUtils::decodeParams(std::move(params));
    params.erase(params.begin());
    StringMap pdict;
    for (auto &kv: params) {
        value = EMailUtils::collapseRFC2231Value(kv.second);
        if (value.size() >= 2 && value.front() == '\"' && value.back() == '\"') {
            value = value.substr(1, value.size() - 2);
        }
        pdict[std::move(kv.first)] = std::move(value);
    }
    return std::make_tuple(std::move(key), std::move(pdict));
}

std::string HTTPUtil::encodeHeader(const std::string &key, const StringMap &pdict) {
    if (pdict.empty()) {
        return key;
    }
    StringVector out{key,};
    for (auto &kv: pdict) {
        if (kv.second.empty()) {
            out.emplace_back(kv.first);
        } else {
            out.emplace_back(kv.first + "=" + kv.second);
        }
    }
    return boost::join(out, "; ");
}


std::string HTTPError::getReason() const {
    auto reason = boost::get_error_info<errinfo_http_reason>(*this);
    if (reason) {
        return *reason;
    }
    auto code = getCode();
    auto iter = HTTP_STATUS_CODES.find(code);
    if (iter != HTTP_STATUS_CODES.end()) {
        return iter->second;
    }
    return "Unknown";
}

bool HTTPError::hasReason() const {
    auto reason = boost::get_error_info<errinfo_http_reason>(*this);
    if (reason) {
        return true;
    }
    auto code = getCode();
    return HTTP_STATUS_CODES.find(code) != HTTP_STATUS_CODES.end();
}

StringVector HTTPError::getCustomErrorInfo() const {
    StringVector errorInfo;
    errorInfo.emplace_back(StrUtil::format("HTTP %d: %s", getCode(), getReason()));
    return errorInfo;
}

NS_END