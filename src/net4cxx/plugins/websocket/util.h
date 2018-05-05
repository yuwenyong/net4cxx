//
// Created by yuwenyong on 18-1-11.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_UTIL_H
#define NET4CXX_PLUGINS_WEBSOCKET_UTIL_H

#include "net4cxx/plugins/websocket/base.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include "net4cxx/common/httputils/urlparse.h"


NS_BEGIN


class NET4CXX_COMMON_API Stopwatch {
public:
    explicit Stopwatch(bool start=true)
            : _elapsed{std::chrono::milliseconds(0)} {
        if (start) {
            _started = TimestampClock::now();
            _running = true;
        } else {
            _started = boost::none;
            _running = false;
        }
    }

    Duration elapsed() const {
        if (_running) {
            return _elapsed + (TimestampClock::now() - *_started);
        } else {
            return _elapsed;
        }
    }

    Duration pause() {
        if (_running) {
            _elapsed += TimestampClock::now() - *_started;
            _running = false;
            return _elapsed;
        } else {
            return _elapsed;
        }
    }

    Duration resume() {
        if (!_running) {
            _started = TimestampClock::now();
            _running = true;
            return _elapsed;
        } else {
            return _elapsed + (TimestampClock::now() - *_started);
        }
    }

    Duration stop() {
        auto elapsed = pause();
        _elapsed = std::chrono::milliseconds(0);
        _started = boost::none;
        _running = false;
        return elapsed;
    }
protected:
    Duration _elapsed;
    boost::optional<Timestamp> _started;
    bool _running;
};


class NET4CXX_COMMON_API Timings {
public:
    void track(const std::string &key) {
        _timings[key] = _stopwatch.elapsed();
    }

    boost::optional<Duration> diffDura(const std::string &startKey, const std::string &endKey);

    boost::optional<double> diff(const std::string &startKey, const std::string &endKey);

    std::string diffFormatted(const std::string &startKey, const std::string &endKey);
protected:
    Stopwatch _stopwatch;
    std::map<std::string, Duration> _timings;
};


class NET4CXX_COMMON_API WebSocketUtil {
public:
    using ParseResult = std::tuple<bool, std::string, unsigned short, std::string, std::string, QueryArgListMap>;

    static ParseResult parseUrl(const std::string &url);

    static std::vector<boost::regex> wildcardsToPatterns(const StringVector &wildcards);
};

//class TrafficStats {
//public:
//    void reset();
//protected:
//    size_t _outgoingOctetsWireLevel{0};
//    size_t _outgoingOctetsWebSocketLevel{0};
//    size_t _outgoingOctetsAppLevel{0};
//    size_t _outgoingWebSocketFrames{0};
//    size_t _outgoingWebSocketMessages{0};
//
//    size_t _incomingOctetsWireLevel{0};
//    size_t _incomingOctetsWebSocketLevel{0};
//    size_t _incomingOctetsAppLevel{0};
//    size_t _incomingWebSocketFrames{0};
//    size_t _incomingWebSocketMessages{0};
//
//    size_t _preopenOutgoingOctetsWireLevel{0};
//    size_t _preopenIncomingOctetsWireLevel{0};
//};
//
//
//class WebSocketOptions {
//public:
//    using Headers = std::map<std::string, StringVector>;
//private:
//    bool _isServer{false};
//    bool _logOctets{false};
//    bool _logFrames{false};
//    bool _trackTimings{false};
//    std::string _url;
//    bool _isSecure{false};
//    std::string _host;
//    unsigned short _port{0};
//    std::string _resource;
//    std::string _path;
//    QueryArgListMap _params;
//    std::string _origin;        // client
//    StringVector _protocols;
//    std::string _server;        // server
//    std::string _useragent;     // client
//    Headers _headers;
//    unsigned short _externalPort{0};    // server
//    std::string _proxy;         // client
//    std::vector<int> _versions; // server
//    int _version{0};            // client
//    bool _webStatus{false};     // server
//    bool _utf8validateIncoming;
//    bool _requireMaskedClientFrames;    // server
//    bool _acceptMaskedServerFrames;     // client
//    bool _maskServerFrames{false};      // server
//    bool _maskClientFrames{false};      // client
//    bool _applyMask{false};
//    size_t _maxFramePayloadSize{0};
//    size_t _maxMessagePayloadSize{0};
//    size_t _autoFragmentSize{0};
//    bool _failByDrop{false};
//    bool _echoCloseCodeReason{false};
//    double _serverConnectionDropTimeout{0.0};   // client
//    double _openHandshakeTimeout{0.0};
//    double _closeHandshakeTimeout{0.0};
//    bool _tcpNoDelay{false};
//    bool _serveFlashSocketPolicy{false};    // server
//    std::string _flashSocketPolicy;         // server
//    // perMessageCompressionOffers          // client
//    // perMessageCompressionAccept
//    double _autoPingInterval{0.0};
//    double _autoPingTimeout{0.0};
//    size_t _autoPingSize{0};
//    StringVector _allowedOrigins;   // server
//    std::vector<boost::regex> _allowedOriginsPatterns;  // server
//    bool _allowNullOrigin{false};   // server
//    size_t _countConnections{0};    // server
//    size_t _maxConnections{0};      // server
//    size_t _trustXForwardedFor{0};  // server
//};

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_UTIL_H
