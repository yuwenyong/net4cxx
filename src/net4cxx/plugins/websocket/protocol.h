//
// Created by yuwenyong on 18-1-11.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_PROTOCOL_H
#define NET4CXX_PLUGINS_WEBSOCKET_PROTOCOL_H

#include "net4cxx/plugins/websocket/base.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include "net4cxx/common/httputils/urlparse.h"
#include "net4cxx/core/network/protocol.h"


NS_BEGIN


//class NET4CXX_COMMON_API WebSocketProtocol: public Protocol {
//public:
//    enum class State {
//        CLOSED = 0,
//        CONNECTING,
//        CLOSING,
//        OPEN,
//        PROXY_CONNECTING,
//    };
//
//    enum class SendState {
//        GROUND = 0,
//        MESSAGE_BEGIN,
//        INSIDE_MESSAGE,
//        INSIDE_MESSAGE_FRAME,
//    };
//
//    explicit WebSocketProtocol(WebSocketOptions *options)
//            : _options(options) {
//
//    }
//
//    void connectionMade() override;
//
//    void dataReceived(Byte *data, size_t length) override;
//
//    void connectionLost(std::exception_ptr reason) override;
//
//protected:
//    std::string getPeerName() const;
//
//    WebSocketOptions *_options{nullptr};
//    std::string _peer{"<never connected>"};
//    bool _tcpNoDelay{false};
//};


class NET4CXX_COMMON_API WebSocketProtocol: public Protocol {
public:
    static const std::vector<int> SUPPORTED_SPEC_VERSIONS;

    static const std::vector<int> SUPPORTED_PROTOCOL_VERSIONS;

    static constexpr int DEFAULT_SPEC_VERSION = 18;
};


class NET4CXX_COMMON_API WebSocketServerFactory: public Factory {
public:
    using Headers = std::map<std::string, StringVector>;

    WebSocketServerFactory(std::string url="", StringVector protocols={}, std::string version="", Headers headers={},
                           unsigned short externalPort=0) {
        setSessionParameters(std::move(url), std::move(protocols), std::move(version), std::move(headers),
                             externalPort);
        resetProtocolOptions();
        _countConnections = 0;
    }

    void setSessionParameters(std::string url="", StringVector protocols={}, std::string server="", Headers headers={},
                              unsigned short externalPort=0);

    void resetProtocolOptions();

    void setVersions(std::vector<int> versions);

    void setWebStatus(bool webStatus) {
        _webStatus = webStatus;
    }

    void setUtf8ValidateIncoming(bool utf8validateIncoming) {
        _utf8validateIncoming = utf8validateIncoming;
    }

    void setRequireMaskedClientFrames(bool requireMaskedClientFrames) {
        _requireMaskedClientFrames = requireMaskedClientFrames;
    }

    void setMaskServerFrames(bool maskServerFrames) {
        _maskServerFrames = maskServerFrames;
    }

    void setApplyMask(bool applyMask) {
        _applyMask = applyMask;
    }

    void setMaxFramePayloadSize(size_t maxFramePayloadSize) {
        _maxFramePayloadSize = maxFramePayloadSize;
    }

    void setMaxMessagePayloadSize(size_t maxMessagePayloadSize) {
        _maxMessagePayloadSize = maxMessagePayloadSize;
    }

    void setAutoFragmentSize(size_t autoFragmentSize) {
        _autoFragmentSize = autoFragmentSize;
    }

    void setFailByDrop(bool failByDrop) {
        _failByDrop = failByDrop;
    }

    void setEchoCloseCodeReason(bool echoCloseCodeReason) {
        _echoCloseCodeReason = echoCloseCodeReason;
    }

    void setOpenHandshakeTimeout(double openHandshakeTimeout) {
        _openHandshakeTimeout = openHandshakeTimeout;
    }

    void setCloseHandshaeTimeout(double closeHandshakeTimeout) {
        _closeHandshakeTimeout = closeHandshakeTimeout;
    }

    void setTcpNoDelay(bool tcpNoDelay) {
        _tcpNoDelay = tcpNoDelay;
    }

    // perMessageCompressionAccept

    void setAutoPingInterval(double autoPingInterval) {
        _autoPingInterval = autoPingInterval;
    }

    void setAutoPingTimeout(double autoPingTimeout) {
        _autoPingTimeout = autoPingTimeout;
    }

    void setAutoPingSize(size_t autoPingSize) {
        NET4CXX_ASSERT(4 <= autoPingSize && autoPingSize <= 125);
        _autoPingSize = autoPingSize;
    }

    void setServerFlashSocketPolicy(bool serverFlashSocketPolicy) {
        _serveFlashSocketPolicy = serverFlashSocketPolicy;
    }

    void setFlashSocketPolicy(std::string flashSocketPolicy) {
        _flashSocketPolicy = std::move(flashSocketPolicy);
    }

    void setAllowedOrigins(StringVector allowedOrigins);

    void setAllowNullOrigin(bool allowNullOrigin) {
        _allowNullOrigin = allowNullOrigin;
    }

    void setMaxConnections(size_t maxConnections) {
        _maxConnections = maxConnections;
    }

    void setTrustXForwardedFor(size_t trustXForwardedFor) {
        _trustXForwardedFor = trustXForwardedFor;
    }

    size_t getConnectionCount() const {
        return _countConnections;
    }
protected:
    bool _logOctets{false};
    bool _logFrames{false};
    bool _trackTimings{false};
    std::string _url;
    bool _isSecure{false};
    std::string _host;
    unsigned short _port{0};
    std::string _resource;
    std::string _path;
    QueryArgListMap _params;
    StringVector _protocols;
    std::string _server;
    Headers _headers;
    unsigned short _externalPort{0};
    std::vector<int> _versions;
    bool _webStatus{false};
    bool _utf8validateIncoming;
    bool _requireMaskedClientFrames;
    bool _maskServerFrames{false};
    bool _applyMask{false};
    size_t _maxFramePayloadSize{0};
    size_t _maxMessagePayloadSize{0};
    size_t _autoFragmentSize{0};
    bool _failByDrop{false};
    bool _echoCloseCodeReason{false};
    double _openHandshakeTimeout{0.0};
    double _closeHandshakeTimeout{0.0};
    bool _tcpNoDelay{false};
    bool _serveFlashSocketPolicy{false};
    std::string _flashSocketPolicy;
    // perMessageCompressionAccept
    double _autoPingInterval{0.0};
    double _autoPingTimeout{0.0};
    size_t _autoPingSize{0};
    StringVector _allowedOrigins;
    std::vector<boost::regex> _allowedOriginsPatterns;
    bool _allowNullOrigin{false};
    size_t _countConnections{0};
    size_t _maxConnections{0};
    size_t _trustXForwardedFor{0};
};


class NET4CXX_COMMON_API WebSocketClientFactory: public ClientFactory {
public:
    using Headers = std::map<std::string, StringVector>;

    WebSocketClientFactory(std::string url = "", std::string origin = "", StringVector protocols = {},
                           std::string useragent = "", Headers headers = {}, std::string proxy = "") {
        setSessionParameters(std::move(url), std::move(origin), std::move(protocols), std::move(useragent),
                             std::move(headers), std::move(proxy));
        resetProtocolOptions();
    }

    void setSessionParameters(std::string url = "", std::string origin = "", StringVector protocols = {},
                              std::string useragent = "", Headers headers = {}, std::string proxy = "");

    void resetProtocolOptions();

    void setVersion(int version);

    void setUtf8ValidateIncoming(bool utf8validateIncoming) {
        _utf8validateIncoming = utf8validateIncoming;
    }

    void setMaskClientFrames(bool maskClientFrames) {
        _maskClientFrames = maskClientFrames;
    }

    void setApplyMask(bool applyMask) {
        _applyMask = applyMask;
    }

    void setMaxFramePayloadSize(size_t maxFramePayloadSize) {
        _maxFramePayloadSize = maxFramePayloadSize;
    }

    void setMaxMessagePayloadSize(size_t maxMessagePayloadSize) {
        _maxMessagePayloadSize = maxMessagePayloadSize;
    }

    void setAutoFragmentSize(size_t autoFragmentSize) {
        _autoFragmentSize = autoFragmentSize;
    }

    void setFailByDrop(bool failByDrop) {
        _failByDrop = failByDrop;
    }

    void setEchoCloseCodeReason(bool echoCloseCodeReason) {
        _echoCloseCodeReason = echoCloseCodeReason;
    }

    void setServerConnectionDropTimeout(double serverConnectionDropTimeout) {
        _serverConnectionDropTimeout = serverConnectionDropTimeout;
    }

    void setOpenHandshakeTimeout(double openHandshakeTimeout) {
        _openHandshakeTimeout = openHandshakeTimeout;
    }

    void setCloseHandshaeTimeout(double closeHandshakeTimeout) {
        _closeHandshakeTimeout = closeHandshakeTimeout;
    }

    void setTcpNoDelay(bool tcpNoDelay) {
        _tcpNoDelay = tcpNoDelay;
    }

    // perMessageCompressionOffers
    // perMessageCompressionAccept

    void setAutoPingInterval(double autoPingInterval) {
        _autoPingInterval = autoPingInterval;
    }

    void setAutoPingTimeout(double autoPingTimeout) {
        _autoPingTimeout = autoPingTimeout;
    }

    void setAutoPingSize(size_t autoPingSize) {
        NET4CXX_ASSERT(4 <= autoPingSize && autoPingSize <= 125);
        _autoPingSize = autoPingSize;
    }
protected:
    bool _logOctets{false};
    bool _logFrames{false};
    bool _trackTimings{false};
    std::string _url;
    bool _isSecure{false};
    std::string _host;
    unsigned short _port{0};
    std::string _resource;
    std::string _path;
    QueryArgListMap _params;
    std::string _origin;
    StringVector _protocols;
    std::string _useragent;
    Headers _headers;
    std::string _proxy;
    int _version{0};
    bool _utf8validateIncoming;
    bool _acceptMaskedServerFrames;
    bool _maskClientFrames{false};
    bool _applyMask{false};
    size_t _maxFramePayloadSize{0};
    size_t _maxMessagePayloadSize{0};
    size_t _autoFragmentSize{0};
    bool _failByDrop{false};
    bool _echoCloseCodeReason{false};
    double _serverConnectionDropTimeout{0.0};
    double _openHandshakeTimeout{0.0};
    double _closeHandshakeTimeout{0.0};
    bool _tcpNoDelay{false};
    // perMessageCompressionOffers
    // perMessageCompressionAccept
    double _autoPingInterval{0.0};
    double _autoPingTimeout{0.0};
    size_t _autoPingSize{0};
};

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_PROTOCOL_H
