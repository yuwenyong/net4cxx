//
// Created by yuwenyong on 18-1-11.
//

#include "net4cxx/plugins/websocket/util.h"


NS_BEGIN

WebSocketUtil::ParseResult WebSocketUtil::parseUrl(const std::string &url) {
    auto parsed = URLParse::urlParse(url);
    const auto &scheme = parsed.getScheme();
    if (scheme != "ws" && scheme != "wss") {
        NET4CXX_THROW_EXCEPTION(Exception, "invalid WebSocket URL: protocol scheme '" + scheme +
                                           "' is not for WebSocket");
    }
    auto hostname = parsed.getHostName();
    if (!parsed.getHostName() || (*hostname).empty()) {
        NET4CXX_THROW_EXCEPTION(Exception, "invalid WebSocket URL: missing hostname");
    }
    auto port = parsed.getPort();
    if (!port) {
        port = scheme == "ws" ? 80 : 443;
    }
    const auto &fragment = parsed.getFragment();
    if (!fragment.empty()) {
        NET4CXX_THROW_EXCEPTION(Exception, "invalid WebSocket URL: non-empty fragment '" + fragment + "'");
    }
    std::string path, ppath;
    if (!parsed.getPath().empty()) {
        ppath = parsed.getPath();
        path = URLParse::unquote(ppath);
    } else {
        ppath = "/";
        path = ppath;
    }
    std::string resource;
    QueryArgListMap params;
    if (!parsed.getQuery().empty()) {
        resource = ppath + "?" + parsed.getQuery();
        params = URLParse::parseQS(parsed.getQuery());
    } else {
        resource = ppath;
    }
    return std::make_tuple(scheme == "wss", std::move(*hostname), *port, std::move(resource), std::move(path),
                           std::move(params));
}

std::vector<boost::regex> WebSocketUtil::wildcardsToPatterns(const StringVector &wildcards) {
    std::vector<boost::regex> patterns;
    for (auto wc: wildcards) {
        boost::replace_all(wc, ".", "\\.");
        boost::replace_all(wc, "*", ".*");
        wc = "^" + wc + "$";
        patterns.emplace_back(wc);
    }
    return patterns;
}

//void TrafficStats::reset() {
//    _outgoingOctetsWireLevel = 0;
//    _outgoingOctetsWebSocketLevel = 0;
//    _outgoingOctetsAppLevel = 0;
//    _outgoingWebSocketFrames = 0;
//    _outgoingWebSocketMessages = 0;
//
//    _incomingOctetsWireLevel = 0;
//    _incomingOctetsWebSocketLevel = 0;
//    _incomingOctetsAppLevel = 0;
//    _incomingWebSocketFrames = 0;
//    _incomingWebSocketMessages = 0;
//
//    _preopenOutgoingOctetsWireLevel = 0;
//    _preopenIncomingOctetsWireLevel = 0;
//}
//


NS_END