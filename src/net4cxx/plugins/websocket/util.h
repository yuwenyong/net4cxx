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
#include "net4cxx/common/utilities/util.h"


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
    using ParseUrlResult = std::tuple<bool, std::string, unsigned short, std::string, std::string, QueryArgListMap>;

    using ParseHttpHeaderResult = std::tuple<std::string, StringMap, std::map<std::string, int>>;

    using UrlToOriginResult = std::tuple<std::string, std::string, boost::optional<unsigned short>>;

    static ParseUrlResult parseUrl(const std::string &url);

    static ParseHttpHeaderResult parseHttpHeader(const std::string &data);

    static UrlToOriginResult urlToOrigin(const std::string &url);

    static bool isSameOrigin(const UrlToOriginResult &websocketOrigin, const std::string &hostScheme,
                             unsigned short hostPort, const std::vector<boost::regex> &hostPolicy);

    static std::vector<boost::regex> wildcardsToPatterns(const StringVector &wildcards);

    static boost::optional<std::string> truncate(const boost::optional<std::string> &text, size_t limit) {
        if (!text) {
            return boost::none;
        }
        if (text->size() > limit) {
            std::string result(text->begin(), text->begin() + limit);
            return result;
        } else {
            return text;
        }
    }

    static ByteArray newid(size_t length=16);
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


class FrameHeader {
public:
    friend class WebSocketProtocol;

    FrameHeader(Byte opcode, bool fin, Byte rsv, uint64_t length, const boost::optional<WebSocketMask> &mask)
            : _opcode(opcode)
            , _fin(fin)
            , _rsv(rsv)
            , _length(length)
            , _mask(mask) {

    }
protected:
    Byte _opcode;
    bool _fin;
    Byte _rsv;
    uint64_t _length;
    boost::optional<WebSocketMask> _mask;
};


class NET4CXX_COMMON_API HexFormatter {
public:
    template <size_t ArrayLen>
    explicit HexFormatter(const std::array<Byte, ArrayLen> &data)
            : HexFormatter(data.data(), data.size()) {

    }

    HexFormatter(const Byte *data, size_t length)
            : _data(data)
            , _length(length) {

    }

    explicit HexFormatter(const ByteArray &data)
            : HexFormatter(data.data(), data.size()) {

    }

    std::string toString() const {
        return BinAscii::hexlify(_data, _length);
    }
protected:
    const Byte* _data;
    size_t _length;
};


NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_UTIL_H
