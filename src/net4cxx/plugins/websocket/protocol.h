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
#include "net4cxx/plugins/websocket/compress.h"
#include "net4cxx/plugins/websocket/util.h"


NS_BEGIN


class NET4CXX_COMMON_API WebSocketProtocol: public Protocol {
public:
    enum class State {
        CLOSED = 0,
        CONNECTING,
        CLOSING,
        OPEN,
        PROXY_CONNECTING,
    };

    enum class SendState {
        GROUND = 0,
        MESSAGE_BEGIN,
        INSIDE_MESSAGE,
        INSIDE_MESSAGE_FRAME,
    };

    void connectionMade() override;

//    void dataReceived(Byte *data, size_t length) override;
//
//    void connectionLost(std::exception_ptr reason) override;

    static const std::vector<int> SUPPORTED_SPEC_VERSIONS;

    static const std::vector<int> SUPPORTED_PROTOCOL_VERSIONS;

    static constexpr int DEFAULT_SPEC_VERSION = 18;
protected:
    std::string getPeerName() const;

    void setTrackTimings(bool enable);

    std::string _peer{"<never connected>"};
    bool _isServer{false};
    // common
    bool _logOctets{false};
    bool _logFrames{false};
    bool _trackTimings{false};
    bool _utf8validateIncomming{false};
    bool _applyMask{false};
    size_t _maxFramePayloadSize{0};
    size_t _maxMessagePayloadSize{0};
    size_t _autoFragmentSize{0};
    bool _failByDrop{false};
    bool _echoCloseCodeReason{false};
    double _openHandshakeTimeout{0.0};
    double _closeHandshakeTimeout{0.0};
    bool _tcpNoDelay{false};
    double _autoPingInterval{0.0};
    double _autoPingTimeout{0.0};
    size_t _autoPingSize{0};
    // server
    std::vector<int> _versions;
    bool _webStatus{false};
    bool _requireMaskedClientFrames{false};
    bool _maskServerFrames{false};
    PerMessageCompressionAccept4Server _perMessageCompressionAccept4Server;
    bool _serverFlashSocketPolicy{false};
    std::string _flashSocketPolicy;
    StringVector _allowedOrigins;
    std::vector<boost::regex> _allowedOriginsPatterns;
    bool _allowNullOrigin{false};
    size_t _maxConnections{0};
    size_t _trustXForwardedFor{0};
    // client
    int _version{0};
    bool _acceptMaskedServerFrames{false};
    bool _maskClientFrames{false};
    double _serverConnectionDropTimeout{0.0};
    std::vector<PerMessageCompressOfferPtr> _perMessageCompressionOffers;
    PerMessageCompressionAccept4Client _perMessageCompressionAccept4Client;
    // extra
    PerMessageCompressPtr _perMessageCompress;
    boost::optional<Timings> _trackedTimings;
    TrafficStats _trafficStats;
    State _state;
    SendState _sendState;
    ByteArray _data;
    std::deque<std::pair<ByteArray, bool>> _sendQueue;
    bool _triggered{false};
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

    void setLogOctets(bool logOctets) {
        _logOctets = logOctets;
    }

    bool getLogOctets() const {
        return _logOctets;
    }

    void setLogFrames(bool logFrames) {
        _logFrames = logFrames;
    }

    bool getLogFrames() const {
        return _logFrames;
    }

    void setTrackTimings(bool trackTimings) {
        _trackTimings = trackTimings;
    }

    bool getTrackTimings() const {
        return _trackTimings;
    }

    void setVersions(std::vector<int> versions);

    std::vector<int> getVersions() const {
        return _versions;
    }

    void setWebStatus(bool webStatus) {
        _webStatus = webStatus;
    }

    bool getWebStatus() const {
        return _webStatus;
    }

    void setUtf8ValidateIncoming(bool utf8validateIncoming) {
        _utf8validateIncoming = utf8validateIncoming;
    }

    bool getUtf8ValidateIncoming() const {
        return _utf8validateIncoming;
    }

    void setRequireMaskedClientFrames(bool requireMaskedClientFrames) {
        _requireMaskedClientFrames = requireMaskedClientFrames;
    }

    bool getRequireMaskedClientFrames() const {
        return _requireMaskedClientFrames;
    }

    void setMaskServerFrames(bool maskServerFrames) {
        _maskServerFrames = maskServerFrames;
    }

    bool getMaskServerFrames() const {
        return _maskServerFrames;
    }

    void setApplyMask(bool applyMask) {
        _applyMask = applyMask;
    }

    bool getApplyMask() const {
        return _applyMask;
    }

    void setMaxFramePayloadSize(size_t maxFramePayloadSize) {
        _maxFramePayloadSize = maxFramePayloadSize;
    }

    size_t getMaxFramePayloadSize() const {
        return _maxFramePayloadSize;
    }

    void setMaxMessagePayloadSize(size_t maxMessagePayloadSize) {
        _maxMessagePayloadSize = maxMessagePayloadSize;
    }

    size_t getMaxMessagePayloadSize() const {
        return _maxMessagePayloadSize;
    }

    void setAutoFragmentSize(size_t autoFragmentSize) {
        _autoFragmentSize = autoFragmentSize;
    }

    size_t getAutoFragmentSize() const {
        return _autoFragmentSize;
    }

    void setFailByDrop(bool failByDrop) {
        _failByDrop = failByDrop;
    }

    bool getFailByDrop() const {
        return _failByDrop;
    }

    void setEchoCloseCodeReason(bool echoCloseCodeReason) {
        _echoCloseCodeReason = echoCloseCodeReason;
    }

    bool getEchoCloseCodeReason() const {
        return _echoCloseCodeReason;
    }

    void setOpenHandshakeTimeout(double openHandshakeTimeout) {
        _openHandshakeTimeout = openHandshakeTimeout;
    }

    double getOpenHandshakeTimeout() const {
        return _openHandshakeTimeout;
    }

    void setCloseHandshakeTimeout(double closeHandshakeTimeout) {
        _closeHandshakeTimeout = closeHandshakeTimeout;
    }

    double getCloseHandshakeTimeout() const {
        return _closeHandshakeTimeout;
    }

    void setTcpNoDelay(bool tcpNoDelay) {
        _tcpNoDelay = tcpNoDelay;
    }

    bool getTcpNoDelay() const {
        return _tcpNoDelay;
    }

    void setPerMessageCompressionAccept(PerMessageCompressionAccept4Server perMessageCompressionAccept) {
        _perMessageCompressionAccept = std::move(perMessageCompressionAccept);
    }

    PerMessageCompressionAccept4Server getPerMessageCompressionAccept() const {
        return _perMessageCompressionAccept;
    }

    void setAutoPingInterval(double autoPingInterval) {
        _autoPingInterval = autoPingInterval;
    }

    double getAutoPingInterval() const {
        return _autoPingInterval;
    }

    void setAutoPingTimeout(double autoPingTimeout) {
        _autoPingTimeout = autoPingTimeout;
    }

    double getAutoPingTimeout() const {
        return _autoPingTimeout;
    }

    void setAutoPingSize(size_t autoPingSize) {
        NET4CXX_ASSERT(4 <= autoPingSize && autoPingSize <= 125);
        _autoPingSize = autoPingSize;
    }

    size_t getAutoPingSize() const {
        return _autoPingSize;
    }

    void setServerFlashSocketPolicy(bool serverFlashSocketPolicy) {
        _serveFlashSocketPolicy = serverFlashSocketPolicy;
    }

    bool getServerFlashSocketPolicy() const {
        return _serveFlashSocketPolicy;
    }

    void setFlashSocketPolicy(std::string flashSocketPolicy) {
        _flashSocketPolicy = std::move(flashSocketPolicy);
    }

    std::string getFlashSocketPolicy() const {
        return _flashSocketPolicy;
    }

    void setAllowedOrigins(StringVector allowedOrigins);

    StringVector getAllowedOrigins() const {
        return _allowedOrigins;
    }

    std::vector<boost::regex> getAllowedOriginsPatterns() const {
        return _allowedOriginsPatterns;
    }

    void setAllowNullOrigin(bool allowNullOrigin) {
        _allowNullOrigin = allowNullOrigin;
    }

    bool getAllowNullOrigin() const {
        return _allowNullOrigin;
    }

    void setMaxConnections(size_t maxConnections) {
        _maxConnections = maxConnections;
    }

    size_t getMaxConnections() const {
        return _maxConnections;
    }

    void setTrustXForwardedFor(size_t trustXForwardedFor) {
        _trustXForwardedFor = trustXForwardedFor;
    }

    size_t getTrustXForwardedFor() const {
        return _trustXForwardedFor;
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
    PerMessageCompressionAccept4Server _perMessageCompressionAccept;
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

    void setLogOctets(bool logOctets) {
        _logOctets = logOctets;
    }

    bool getLogOctets() const {
        return _logOctets;
    }

    void setLogFrames(bool logFrames) {
        _logFrames = logFrames;
    }

    bool getLogFrames() const {
        return _logFrames;
    }

    void setTrackTimings(bool trackTimings) {
        _trackTimings = trackTimings;
    }

    bool getTrackTimings() const {
        return _trackTimings;
    }

    std::string getProxy() const {
        return _proxy;
    }

    void setVersion(int version);

    int getVersion() const {
        return _version;
    }

    void setUtf8ValidateIncoming(bool utf8validateIncoming) {
        _utf8validateIncoming = utf8validateIncoming;
    }

    bool getUtf8ValidateIncoming() const {
        return _utf8validateIncoming;
    }

    void setAcceptMaskedServerFrames(bool acceptMaskedServerFrames) {
        _acceptMaskedServerFrames = acceptMaskedServerFrames;
    }

    bool getAcceptMaskedServerFrames() const {
        return _acceptMaskedServerFrames;
    }

    void setMaskClientFrames(bool maskClientFrames) {
        _maskClientFrames = maskClientFrames;
    }

    bool getMaskClientFrames() const {
        return _maskClientFrames;
    }

    void setApplyMask(bool applyMask) {
        _applyMask = applyMask;
    }

    bool getApplyMask() const {
        return _applyMask;
    }

    void setMaxFramePayloadSize(size_t maxFramePayloadSize) {
        _maxFramePayloadSize = maxFramePayloadSize;
    }

    size_t getMaxFramePayloadSize() const {
        return _maxFramePayloadSize;
    }

    void setMaxMessagePayloadSize(size_t maxMessagePayloadSize) {
        _maxMessagePayloadSize = maxMessagePayloadSize;
    }

    size_t getMaxMessagePayloadSize() const {
        return _maxMessagePayloadSize;
    }

    void setAutoFragmentSize(size_t autoFragmentSize) {
        _autoFragmentSize = autoFragmentSize;
    }

    size_t getAutoFragmentSize() const {
        return _autoFragmentSize;
    }

    void setFailByDrop(bool failByDrop) {
        _failByDrop = failByDrop;
    }

    bool getFailByDrop() const {
        return _failByDrop;
    }

    void setEchoCloseCodeReason(bool echoCloseCodeReason) {
        _echoCloseCodeReason = echoCloseCodeReason;
    }

    bool getEchoCloseCodeReason() const {
        return _echoCloseCodeReason;
    }

    void setServerConnectionDropTimeout(double serverConnectionDropTimeout) {
        _serverConnectionDropTimeout = serverConnectionDropTimeout;
    }

    double getServerConnectionDropTimeout() const {
        return _serverConnectionDropTimeout;
    }

    void setOpenHandshakeTimeout(double openHandshakeTimeout) {
        _openHandshakeTimeout = openHandshakeTimeout;
    }

    double getOpenHandshakeTimeout() const {
        return _openHandshakeTimeout;
    }

    void setCloseHandshaeTimeout(double closeHandshakeTimeout) {
        _closeHandshakeTimeout = closeHandshakeTimeout;
    }

    double getCloseHandshakeTimeout() const {
        return _closeHandshakeTimeout;
    }

    void setTcpNoDelay(bool tcpNoDelay) {
        _tcpNoDelay = tcpNoDelay;
    }

    bool getTcpNoDelay() const {
        return _tcpNoDelay;
    }

    void setPerMessageCompressionOffers(std::vector<PerMessageCompressOfferPtr> perMessageCompressionOffers) {
        _perMessageCompressionOffers = std::move(perMessageCompressionOffers);
    }

    std::vector<PerMessageCompressOfferPtr> getPerMessageCompressionOffers() const {
        return _perMessageCompressionOffers;
    }

    void setPerMessageCompressionAccept(PerMessageCompressionAccept4Client perMessageCompressionAccept) {
        _perMessageCompressionAccept = std::move(perMessageCompressionAccept);
    }

    PerMessageCompressionAccept4Client getPerMessageCompressionAccept() const {
        return _perMessageCompressionAccept;
    }

    void setAutoPingInterval(double autoPingInterval) {
        _autoPingInterval = autoPingInterval;
    }

    double getAutoPingInterval() const {
        return _autoPingInterval;
    }

    void setAutoPingTimeout(double autoPingTimeout) {
        _autoPingTimeout = autoPingTimeout;
    }

    double getAutoPingTimeout() const {
        return _autoPingTimeout;
    }

    void setAutoPingSize(size_t autoPingSize) {
        NET4CXX_ASSERT(4 <= autoPingSize && autoPingSize <= 125);
        _autoPingSize = autoPingSize;
    }

    size_t getAutoPingSize() const {
        return _autoPingSize;
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
    std::vector<PerMessageCompressOfferPtr> _perMessageCompressionOffers;
    PerMessageCompressionAccept4Client _perMessageCompressionAccept;
    double _autoPingInterval{0.0};
    double _autoPingTimeout{0.0};
    size_t _autoPingSize{0};
};

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_PROTOCOL_H
