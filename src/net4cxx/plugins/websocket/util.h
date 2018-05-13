//
// Created by yuwenyong on 18-1-11.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_UTIL_H
#define NET4CXX_PLUGINS_WEBSOCKET_UTIL_H

#include "net4cxx/plugins/websocket/base.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include "net4cxx/common/configuration/json.h"
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


class TrafficStats {
public:
    friend class WebSocketProtocol;

    void reset();

    JSONValue toJson() const;

    std::string toString() const;
protected:
    size_t _outgoingOctetsWireLevel{0};
    size_t _outgoingOctetsWebSocketLevel{0};
    size_t _outgoingOctetsAppLevel{0};
    size_t _outgoingWebSocketFrames{0};
    size_t _outgoingWebSocketMessages{0};

    size_t _incomingOctetsWireLevel{0};
    size_t _incomingOctetsWebSocketLevel{0};
    size_t _incomingOctetsAppLevel{0};
    size_t _incomingWebSocketFrames{0};
    size_t _incomingWebSocketMessages{0};

    size_t _preopenOutgoingOctetsWireLevel{0};
    size_t _preopenIncomingOctetsWireLevel{0};
};


NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_UTIL_H
