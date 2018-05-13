//
// Created by yuwenyong on 18-1-11.
//

#include "net4cxx/plugins/websocket/protocol.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN

const std::vector<int> WebSocketProtocol::SUPPORTED_SPEC_VERSIONS = {10, 11, 12, 13, 14, 15, 16, 17, 18};

const std::vector<int> WebSocketProtocol::SUPPORTED_PROTOCOL_VERSIONS = {8, 13};

constexpr int WebSocketProtocol::DEFAULT_SPEC_VERSION;

void WebSocketProtocol::connectionMade() {
    _peer = getPeerName();

    _isServer = std::dynamic_pointer_cast<WebSocketServerFactory>(_factory.lock()) ? true: false;
    if (_isServer) {
        auto factory = getFactory<WebSocketServerFactory>();
        NET4CXX_ASSERT(factory);
        _logOctets = factory->getLogOctets();
        _logFrames = factory->getLogFrames();
        _trackTimings = factory->getTrackTimings();
        _utf8validateIncomming = factory->getUtf8ValidateIncoming();
        _applyMask = factory->getApplyMask();
        _maxFramePayloadSize = factory->getMaxFramePayloadSize();
        _maxMessagePayloadSize = factory->getMaxMessagePayloadSize();
        _autoFragmentSize = factory->getAutoFragmentSize();
        _failByDrop = factory->getFailByDrop();
        _echoCloseCodeReason = factory->getEchoCloseCodeReason();
        _openHandshakeTimeout = factory->getOpenHandshakeTimeout();
        _closeHandshakeTimeout = factory->getCloseHandshakeTimeout();
        _tcpNoDelay = factory->getTcpNoDelay();
        _autoPingInterval = factory->getAutoPingInterval();
        _autoPingTimeout = factory->getAutoPingTimeout();
        _autoPingSize = factory->getAutoPingSize();

        _versions = factory->getVersions();
        _webStatus = factory->getWebStatus();
        _requireMaskedClientFrames = factory->getRequireMaskedClientFrames();
        _maskServerFrames = factory->getMaskServerFrames();
        _perMessageCompressionAccept4Server = factory->getPerMessageCompressionAccept();
        _serverFlashSocketPolicy = factory->getServerFlashSocketPolicy();
        _flashSocketPolicy = factory->getFlashSocketPolicy();
        _allowedOrigins = factory->getAllowedOrigins();
        _allowedOriginsPatterns = factory->getAllowedOriginsPatterns();
        _allowNullOrigin = factory->getAllowNullOrigin();
        _maxConnections = factory->getMaxConnections();
        _trustXForwardedFor = factory->getTrustXForwardedFor();

        _state = State::CONNECTING;
    } else {
        auto factory = getFactory<WebSocketClientFactory>();
        NET4CXX_ASSERT(factory);
        _logOctets = factory->getLogOctets();
        _logFrames = factory->getLogFrames();
        _trackTimings = factory->getTrackTimings();
        _utf8validateIncomming = factory->getUtf8ValidateIncoming();
        _applyMask = factory->getApplyMask();
        _maxFramePayloadSize = factory->getMaxFramePayloadSize();
        _maxMessagePayloadSize = factory->getMaxMessagePayloadSize();
        _autoFragmentSize = factory->getAutoFragmentSize();
        _failByDrop = factory->getFailByDrop();
        _echoCloseCodeReason = factory->getEchoCloseCodeReason();
        _openHandshakeTimeout = factory->getOpenHandshakeTimeout();
        _closeHandshakeTimeout = factory->getCloseHandshakeTimeout();
        _tcpNoDelay = factory->getTcpNoDelay();
        _autoPingInterval = factory->getAutoPingInterval();
        _autoPingTimeout = factory->getAutoPingTimeout();
        _autoPingSize = factory->getAutoPingSize();

        _version = factory->getVersion();
        _acceptMaskedServerFrames = factory->getAcceptMaskedServerFrames();
        _maskClientFrames = factory->getMaskClientFrames();
        _serverConnectionDropTimeout = factory->getServerConnectionDropTimeout();
        _perMessageCompressionAccept4Client = factory->getPerMessageCompressionAccept();
        _perMessageCompressionOffers = factory->getPerMessageCompressionOffers();

        if (factory->getProxy().empty()) {
            _state = State::CONNECTING;
        } else {
            _state = State::PROXY_CONNECTING;
        }
    }

    setTrackTimings(_trackTimings);
    _sendState = SendState::GROUND;

    NET4CXX_LOG_DEBUG(gGenLog, "Connection made to %s", _peer.c_str());
    setNoDelay(_tcpNoDelay);

    if (_openHandshakeTimeout > 0.0) {
        _openHandshakeTimeoutCall = reactor()->callLater(_openHandshakeTimeout, [self = shared_from_this(), this](){
            onOpenHandshakeTimeout();
        });
    }
}

void WebSocketProtocol::dataReceived(Byte *data, size_t length) {
    if (_state == State::OPEN) {
        _trafficStats._incomingOctetsWireLevel += length;
    } else if (_state == State::CONNECTING || _state == State::PROXY_CONNECTING) {
        _trafficStats._preopenIncomingOctetsWireLevel += length;
    }

    if (_logOctets) {
        logRxOctets(data, length);
    }
    _data.insert(_data.end(), data, data + length);
    consumeData();
}

//void WebSocketProtocol::connectionLost(std::exception_ptr reason) {
//
//}

std::string WebSocketProtocol::getPeerName() const {
    std::string res;
    auto address = getRemoteAddress();
    if (NetUtil::isValidIPv4(address)) {
        res = "tcp4:" + address + std::to_string(getRemotePort());
    } else if (NetUtil::isValidIPv6(address)) {
        res = "tcp6:" + address + std::to_string(getRemotePort());
    } else if (!address.empty()) {
        res = "unix:" + address;
    } else {
        res = "?:";
    }
    return res;
}

void WebSocketProtocol::setTrackTimings(bool enable) {
    if (enable) {
        _trackedTimings = Timings{};
    } else {
        _trackedTimings = boost::none;
    }
}

void WebSocketProtocol::onOpenHandshakeTimeout() {
    if (_state == State::CONNECTING || _state == State::PROXY_CONNECTING) {
        _wasClean = false;
        _wasNotCleanReason = "WebSocket opening handshake timeout (peer did not finish the opening handshake in time)";
        _wasOpenHandshakeTimeout = true;
        dropConnection(true);
    } else if (_state == State::OPEN){
        NET4CXX_LOG_DEBUG(gGenLog, "skipping opening handshake timeout: WebSocket connection is open "
                                   "(opening handshake already finished)");
    } else if (_state == State::CLOSING) {
        NET4CXX_LOG_DEBUG(gGenLog, "skipping opening handshake timeout: WebSocket connection is already closing ..");
    } else if (_state == State::CLOSED) {
        NET4CXX_LOG_DEBUG(gGenLog, "skipping opening handshake timeout: WebSocket connection is already closed");
    } else {
        NET4CXX_ASSERT_MSG(false, "logic error");
    }
}

void WebSocketProtocol::dropConnection(bool abort) {
    if (_state != State::CLOSED) {
        if (_wasClean) {
            NET4CXX_LOG_DEBUG(gGenLog, "dropping connection to peer %s with abort=%s", _peer.c_str(),
                              abort ? "true" : "false");
        } else {
            NET4CXX_LOG_WARN(gGenLog, "dropping connection to peer %s with abort=%s: %s", _peer.c_str(),
                             abort ? "true" : "false", _wasNotCleanReason ? _wasNotCleanReason->c_str() : "None");
        }
        _droppedByMe = true;
        _state = State::CLOSED;
        closeConnection(abort);
    } else {
        NET4CXX_LOG_DEBUG(gGenLog, "dropping connection to peer %s skipped - connection already closed", _peer.c_str());
    }
}

void WebSocketProtocol::consumeData() {

}


void WebSocketServerFactory::setSessionParameters(std::string url, StringVector protocols, std::string server,
                                                  Headers headers, unsigned short externalPort) {
    if (url.empty()) {
        url = "ws://localhost";
    }
    if (server.empty()) {
        server = NET4CXX_VER;
    }
    bool isSecure;
    std::string host, resource, path;
    unsigned short port;
    QueryArgListMap params;
    std::tie(isSecure, host, port, resource, path, params) = WebSocketUtil::parseUrl(url);
    if (!params.empty()) {
        NET4CXX_THROW_EXCEPTION(Exception, "query parameters specified for server WebSocket URL");
    }
    _url = std::move(url);
    _isSecure = isSecure;
    _host = std::move(host);
    _resource = std::move(resource);
    _path = std::move(path);
    _params = std::move(params);

    _protocols = std::move(protocols);
    _server = std::move(server);
    _headers = std::move(headers);

    if (externalPort) {
        _externalPort = externalPort;
    } else if (!_url.empty()) {
        _externalPort = _port;
    } else {
        _externalPort = 0;
    }
}

void WebSocketServerFactory::resetProtocolOptions() {
    _versions = WebSocketProtocol::SUPPORTED_PROTOCOL_VERSIONS;
    _webStatus = true;
    _utf8validateIncoming = true;
    _requireMaskedClientFrames = true;
    _maskServerFrames = false;
    _applyMask = true;
    _maxFramePayloadSize = 0;
    _maxMessagePayloadSize = 0;
    _autoFragmentSize = 0;
    _failByDrop = true;
    _echoCloseCodeReason = false;
    _openHandshakeTimeout = 5.0;
    _closeHandshakeTimeout = 1.0;
    _tcpNoDelay = true;
    _serveFlashSocketPolicy = false;
    _flashSocketPolicy = R"(<cross-domain-policy>
  <allow-access-from domain="*" to-ports="*" />
</cross-domain-policy>)";
    _flashSocketPolicy.append(1, '\0');

    _perMessageCompressionAccept = nullptr;

    _autoPingInterval = 0.0;
    _autoPingTimeout = 0.0;
    _autoPingSize = 4;

    _allowedOrigins = {"*"};
    _allowedOriginsPatterns = WebSocketUtil::wildcardsToPatterns(_allowedOrigins);
    _allowNullOrigin = true;

    _maxConnections =  0;

    _trustXForwardedFor = 0;
};

void WebSocketServerFactory::setVersions(std::vector<int> versions) {
    for (auto v: versions) {
        if (!std::binary_search(WebSocketProtocol::SUPPORTED_PROTOCOL_VERSIONS.begin(),
                                WebSocketProtocol::SUPPORTED_PROTOCOL_VERSIONS.end(), v)) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid WebSocket protocol version " + std::to_string(v));
        }
    }
    _versions = std::move(versions);
}

void WebSocketServerFactory::setAllowedOrigins(StringVector allowedOrigins) {
    _allowedOrigins = std::move(allowedOrigins);
    _allowedOriginsPatterns = WebSocketUtil::wildcardsToPatterns(_allowedOrigins);
}


void WebSocketClientFactory::setSessionParameters(std::string url, std::string origin, StringVector protocols,
                                                  std::string useragent, Headers headers, std::string proxy) {
    if (url.empty()) {
        url = "ws://localhost";
    }
    if (useragent.empty()) {
        useragent = NET4CXX_VER;
    }
    bool isSecure;
    std::string host, resource, path;
    unsigned short port;
    QueryArgListMap params;
    std::tie(isSecure, host, port, resource, path, params) = WebSocketUtil::parseUrl(url);
    _url = std::move(url);
    _isSecure = isSecure;
    _host = std::move(host);
    _resource = std::move(resource);
    _path = std::move(path);
    _params = std::move(params);

    _origin = std::move(origin);
    _protocols = std::move(protocols);
    _useragent = std::move(useragent);

    _proxy = std::move(proxy);
}

void WebSocketClientFactory::resetProtocolOptions() {
    _version = WebSocketProtocol::DEFAULT_SPEC_VERSION;
    _utf8validateIncoming = true;
    _acceptMaskedServerFrames = false;
    _maskClientFrames = true;
    _applyMask = true;
    _maxFramePayloadSize = 0;
    _maxMessagePayloadSize = 0;
    _autoFragmentSize = 0;
    _failByDrop = true;
    _echoCloseCodeReason = false;
    _serverConnectionDropTimeout = 1.0f;
    _openHandshakeTimeout = 5.0f;
    _closeHandshakeTimeout = 1.0f;
    _tcpNoDelay = true;

    _perMessageCompressionOffers.clear();
    _perMessageCompressionAccept = nullptr;

    _autoPingInterval = 0.0f;
    _autoPingTimeout = 0.0f;
    _autoPingSize = 4;
}

void WebSocketClientFactory::setVersion(int version) {
    if (!std::binary_search(WebSocketProtocol::SUPPORTED_SPEC_VERSIONS.begin(),
                            WebSocketProtocol::SUPPORTED_SPEC_VERSIONS.end(), version)) {
        NET4CXX_THROW_EXCEPTION(Exception, "invalid WebSocket draft version " + std::to_string(version));
    }
    _version = version;
}

NS_END