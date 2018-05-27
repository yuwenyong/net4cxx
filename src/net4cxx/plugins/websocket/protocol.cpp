//
// Created by yuwenyong on 18-1-11.
//

#include "net4cxx/plugins/websocket/protocol.h"
#include <boost/endian/conversion.hpp>
#include "net4cxx/common/utilities/random.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN

const std::vector<int> WebSocketProtocol::SUPPORTED_SPEC_VERSIONS = {10, 11, 12, 13, 14, 15, 16, 17, 18};

const std::vector<int> WebSocketProtocol::SUPPORTED_PROTOCOL_VERSIONS = {8, 13};

const std::vector<unsigned short> WebSocketProtocol::CLOSE_STATUS_CODES_ALLOWED = {
        CLOSE_STATUS_CODE_NORMAL,
        CLOSE_STATUS_CODE_GOING_AWAY,
        CLOSE_STATUS_CODE_PROTOCOL_ERROR,
        CLOSE_STATUS_CODE_UNSUPPORTED_DATA,
        CLOSE_STATUS_CODE_INVALID_PAYLOAD,
        CLOSE_STATUS_CODE_POLICY_VIOLATION,
        CLOSE_STATUS_CODE_MESSAGE_TOO_BIG,
        CLOSE_STATUS_CODE_MANDATORY_EXTENSION,
        CLOSE_STATUS_CODE_INTERNAL_ERROR,
        CLOSE_STATUS_CODE_SERVICE_RESTART,
        CLOSE_STATUS_CODE_TRY_AGAIN_LATER
};

constexpr int WebSocketProtocol::DEFAULT_SPEC_VERSION;

constexpr unsigned WebSocketProtocol::MESSAGE_TYPE_TEXT;

constexpr unsigned WebSocketProtocol::MESSAGE_TYPE_BINARY;

const double WebSocketProtocol::QUEUED_WRITE_DELAY = 0.00001;

void WebSocketProtocol::onOpen() {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onOpen");
}

void WebSocketProtocol::onMessage(ByteArray payload, bool isBinary) {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onMessage(payload=<%llu bytes)>, isBinary=%s",
                      (uint64_t)payload.size(), isBinary ? "true" : "false");
}

void WebSocketProtocol::onPing(ByteArray payload) {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onPing(payload=<%llu bytes>)", (uint64_t)payload.size());
    if (_state == State::OPEN) {
        sendPong(payload.data(), payload.size());
    }
}

void WebSocketProtocol::onPong(ByteArray payload) {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onPong(payload=<%llu bytes>)", (uint64_t)payload.size());
}

void WebSocketProtocol::onClose(bool wasClean, boost::optional<unsigned short> code,
                                boost::optional<std::string> reason) {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onClose(wasClean=%s, code=%u, reason=%s)",
                      wasClean ? "true" : "false", code ? *code : 0u, reason ? reason->c_str() : "None");
}

void WebSocketProtocol::sendMessage(const Byte *payload, size_t length, bool isBinary, size_t fragmentSize, bool sync,
                                    bool doNotCompress) {
    if (_state != State::OPEN) {
        return;
    }

    if (_trackedTimings) {
        _trackedTimings->track("sendMessage");
    }

    // Byte opcode = isBinary ? 2 : 1;
    // bool sendCompressed;

    _trafficStats._outgoingWebSocketMessages += 1;

    if (_perMessageCompress && !doNotCompress) {
        _perMessageCompress->startCompressMessage();
        _trafficStats._outgoingOctetsAppLevel += length;


    }
}

void WebSocketProtocol::sendPing(const Byte *payload, size_t length) {
    if (_state != State::OPEN) {
        return;
    }
    if (length != 0) {
        uint64_t l = length;
        if (l > 125u) {
            NET4CXX_THROW_EXCEPTION(Exception, StrUtil::format("invalid payload for PING (payload length must "
                                                               "be <= 125, was %llu)", l));
        }
        sendFrame(9u, payload, length);
    } else {
        sendFrame(9u);
    }
}

void WebSocketProtocol::sendPong(const Byte *payload, size_t length) {
    if (_state != State::OPEN) {
        return;
    }
    if (length != 0) {
        uint64_t l = length;
        if (l > 125u) {
            NET4CXX_THROW_EXCEPTION(Exception, StrUtil::format("invalid payload for PONG (payload length must "
                                                               "be <= 125, was %llu)", l));
        }
        sendFrame(10u, payload, length);
    } else {
        sendFrame(10u);
    }
}

void WebSocketProtocol::sendClose(boost::optional<unsigned short> code, boost::optional<std::string> reason) {
    if (code) {
        if (*code != 1000u && !(3000u <= *code && *code <= 4999u)) {
            NET4CXX_THROW_EXCEPTION(Exception, StrUtil::format("invalid close code %u (must be "
                                                               "1000 or from [3000, 4999])", *code));
        }
    }
    boost::optional<std::string> reasonUtf8;
    if (reason) {
        if (!code) {
            NET4CXX_THROW_EXCEPTION(Exception, "close reason without close code");
        }
        reasonUtf8 = WebSocketUtil::truncate(reason, 123);
    }
    sendCloseFrame(code, std::move(reasonUtf8), false);
}

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

void WebSocketProtocol::connectionLost(std::exception_ptr reason) {
    try {
        std::rethrow_exception(reason);
    } catch (ConnectionDone &e) {
        NET4CXX_LOG_DEBUG(gGenLog, "Connection to/from %s was closed cleanly", _peer.c_str());
    } catch (ConnectionAbort &e) {
        NET4CXX_LOG_DEBUG(gGenLog, "Connection to/from %s was aborted locally", _peer.c_str());
    } catch (std::exception &e) {
        NET4CXX_LOG_DEBUG(gGenLog, "Connection to/from %s lost: %s", _peer.c_str(), e.what());
    }

    if (!_isServer && !_serverConnectionDropTimeoutCall.cancelled()) {
        NET4CXX_LOG_DEBUG(gGenLog, "serverConnectionDropTimeoutCall.cancel");
        _serverConnectionDropTimeoutCall.cancel();
    }

    if (!_autoPingPendingCall.cancelled()) {
        NET4CXX_LOG_DEBUG(gGenLog,"Auto ping/pong: canceling autoPingPendingCall upon lost connection");
        _autoPingPendingCall.cancel();
    }

    if (_state != State::CLOSED) {
        _state = State::CLOSED;
        // todo: trigger close event
    }

    if (_wasServingFlashSocketPolicyFile) {
        NET4CXX_LOG_DEBUG(gGenLog, "connection dropped after serving Flash Socket Policy File");
    } else {
        if (!_wasClean) {
            if (!_droppedByMe && !_wasNotCleanReason) {
                _wasNotCleanReason = "peer dropped the TCP connection without previous WebSocket closing handshake";
            }
            onClose(_wasClean, CLOSE_STATUS_CODE_ABNORMAL_CLOSE, StrUtil::format("connection was closed uncleanly (%s)",
                                                                                 _wasNotCleanReason->c_str()));
        } else {
            onClose(_wasClean, _remoteCloseCode, *_remoteCloseReason);
        }
    }
}

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

void WebSocketProtocol::onServerConnectionDropTimeout() {
    _serverConnectionDropTimeoutCall.reset();

    if (_state != State::CLOSED) {
        _wasClean = false;
        _wasNotCleanReason = "WebSocket closing handshake timeout (server did not drop TCP connection in time)";
        _wasServerConnectionDropTimeout = true;
        dropConnection(true);
    } else {
        NET4CXX_LOG_DEBUG(gGenLog, "skipping closing handshake timeout: server did indeed drop the connection in time");
    }
}

void WebSocketProtocol::onOpenHandshakeTimeout() {
    _openHandshakeTimeoutCall.reset();

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

void WebSocketProtocol::onCloseHandshakeTimeout() {
    _closeHandshakeTimeoutCall.reset();

    if (_state != State::CLOSED) {
        _wasClean = false;
        _wasNotCleanReason = "WebSocket closing handshake timeout (peer did not finish the opening handshake in time)";
        _wasCloseHandshakeTimeout = true;
        dropConnection(true);
    } else {
        NET4CXX_LOG_DEBUG(gGenLog, "skipping closing handshake timeout: WebSocket connection is already closed");
    }
}

void WebSocketProtocol::onAutoPingTimeout() {
    _wasClean = false;
    _wasNotCleanReason = "WebSocket ping timeout (peer did not respond with pong in time)";
    _autoPingTimeoutCall.reset();
    dropConnection(true);
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
    if (_state == State::OPEN || _state == State::CLOSING) {
        while (processData() && _state != State::CLOSED) {

        }
    } else if (_state == State::PROXY_CONNECTING) {
        processProxyConnect();
    } else if (_state == State::CONNECTING) {
        processHandshake();
    } else if (_state == State::CLOSED) {
        NET4CXX_LOG_DEBUG(gGenLog, "received data in STATE_CLOSED");
    } else {
        NET4CXX_ASSERT_MSG(false, "invalid state");
    }
}

bool WebSocketProtocol::processData() {
    size_t bufferedLen = _data.size();
    if (!_currentFrame) {
        if (bufferedLen >= 2) {
            Byte b = _data[0];
            bool frameFin = (b & 0x80u) != 0;
            Byte frameRsv = (b & 0x70u) >> 4u;
            Byte frameOpcode = b & 0x0fu;

            b = _data[1];
            bool frameMasked = (b & 0x80u) != 0;
            Byte framePayloadLen1 = (b & 0x7fu);

            if (frameRsv != 0) {
                if (_perMessageCompress && frameRsv == 4) {

                } else {
                    if (protocolViolation(StrUtil::format("RSV = %u and no extension negotiated",
                                                          (unsigned int)frameRsv))) {
                        return false;
                    }
                }
            }

            if (_isServer && _requireMaskedClientFrames && !frameMasked) {
                if (protocolViolation("unmasked client-to-server frame")) {
                    return false;
                }
            }

            if (!_isServer && !_acceptMaskedServerFrames && frameMasked) {
                if (protocolViolation("masked server-to-client frame")) {
                    return false;
                }
            }

            if (frameOpcode > 7u) { // control frame
                if (!frameFin) {
                    if (protocolViolation("fragmented control frame")) {
                        return false;
                    }
                }

                if (framePayloadLen1 > 125u) {
                    if (protocolViolation("control frame with payload length > 125 octets")) {
                        return false;
                    }
                }

                if (frameOpcode != 8u && frameOpcode != 9u && frameOpcode != 10u) {
                    if (protocolViolation(StrUtil::format("control frame using reserved opcode %u",
                                                          (unsigned int)frameOpcode))) {
                        return false;
                    }
                }

                if (frameOpcode == 8u && framePayloadLen1 == 1u) {
                    if (protocolViolation("received close control frame with payload len 1")) {
                        return false;
                    }
                }

                if (_perMessageCompress && frameRsv == 4) {
                    if (protocolViolation(StrUtil::format("received compressed control frame [%s]",
                                                          _perMessageCompress->getExtensionName().c_str()))) {
                        return false;
                    }
                }
            } else { // data frame
                if (frameOpcode != 0u && frameOpcode != 1u && frameOpcode != 2u) {
                    if (protocolViolation(StrUtil::format("data frame using reserved opcode %u",
                                                          (unsigned int)frameOpcode))) {
                        return false;
                    }
                }
                if (!_insideMessage && frameOpcode == 0u) {
                    if (protocolViolation("received continuation data frame outside fragmented message")) {
                        return false;
                    }
                }
                if (_insideMessage && frameOpcode != 0u) {
                    if (protocolViolation("received non-continuation data frame while inside fragmented message")) {
                        return false;
                    }
                }
                if (_perMessageCompress && frameRsv == 4u && _insideMessage) {
                    if (protocolViolation(StrUtil::format("received continuation data frame with compress bit set [%s]",
                                                          _perMessageCompress->getExtensionName().c_str()))) {
                        return false;
                    }
                }
            }

            size_t maskLen = frameMasked ? 4u : 0u;
            size_t frameHeaderLen;
            if (framePayloadLen1 < 126u) {
                frameHeaderLen = 2 + maskLen;
            } else if (framePayloadLen1 == 126u) {
                frameHeaderLen = 2 + 2 + maskLen;
            } else {
                frameHeaderLen = 2 + 8 + maskLen;
            }

            if (bufferedLen >=  frameHeaderLen) {
                size_t i = 2;
                uint64_t framePayloadLen;
                if (framePayloadLen1 == 126u) {
                    framePayloadLen = boost::endian::big_to_native(*(uint16_t *)(_data.data() + i));
                    if (framePayloadLen < 126u) {
                        if (protocolViolation("invalid data frame length (not using minimal length encoding)")) {
                            return false;
                        }
                    }
                    i += 2;
                } else if (framePayloadLen1 == 127u) {
                    framePayloadLen = boost::endian::big_to_native(*(uint64_t *)(_data.data() + i));
                    if (framePayloadLen > 0x7FFFFFFFFFFFFFFFu) {
                        if (protocolViolation("invalid data frame length (>2^63)")) {
                            return false;
                        }
                    }
                    if (framePayloadLen < 65536u) {
                        if (protocolViolation("invalid data frame length (not using minimal length encoding)")) {
                            return false;
                        }
                    }
                    i += 8;
                } else {
                    framePayloadLen = framePayloadLen1;
                }
                WebSocketMask frameMask;
                if (frameMasked) {
                    std::copy(_data.begin() + i, _data.begin() + i + 4, frameMask.begin());
                    i += 4;
                }
                if (frameMasked && framePayloadLen > 0 && _applyMask) {
                    _currentFrameMasker = createXorMasker(frameMask, framePayloadLen);
                } else {
                    _currentFrameMasker = std::make_unique<XorMaskerNull>();
                }
                _data.erase(_data.begin(), _data.begin() + i);
                _currentFrame = FrameHeader(frameOpcode, frameFin, frameRsv, framePayloadLen, frameMask);
                onFrameBegin();
                return framePayloadLen == 0 || !_data.empty();
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        uint64_t rest = _currentFrame->_length - _currentFrameMasker->pointer();
        ByteArray payload;
        if (bufferedLen >= rest) {
            payload.assign(_data.begin(), _data.begin() + rest);
            _data.erase(_data.begin(), _data.begin() + rest);
        } else {
            payload = std::move(_data);
        }

        if (!payload.empty()) {
            _currentFrameMasker->process(payload.data(), payload.size());
        }
        if (!onFrameData(std::move(payload))) {
            return false;
        }
        if (_currentFrameMasker->pointer() == _currentFrame->_length) {
            if (!onFrameEnd()) {
                return false;
            }
        }
        return !_data.empty();
    }
}

void WebSocketProtocol::failConnection(unsigned short code, const std::string &reason) {
    if (_state != State::CLOSED) {
        NET4CXX_LOG_DEBUG(gGenLog, "failing connection: %u: %s", code, reason.c_str());
        _failedByMe = true;
        if (_failByDrop) {
            _wasClean = false;
            _wasNotCleanReason = "I dropped the WebSocket TCP connection: " + reason;
            dropConnection(true);
        } else {
            if (_state != State::CLOSING) {
                sendCloseFrame(code, WebSocketUtil::truncate(reason, 123), false);
            } else {
                dropConnection(false);
            }
        }
    } else {
        NET4CXX_LOG_DEBUG(gGenLog, "skip failing of connection since connection is already closed");
    }
}

void WebSocketProtocol::sendCloseFrame(boost::optional<unsigned short> code, boost::optional<std::string> reason,
                                       bool isReply) {
    if (_state == State::CLOSING) {
        NET4CXX_LOG_DEBUG(gGenLog, "ignoring sendCloseFrame since connection is closing");
    } else if (_state == State::CLOSED) {
        NET4CXX_LOG_DEBUG(gGenLog, "ignoring sendCloseFrame since connection already closed");
    } else if (_state == State::PROXY_CONNECTING || _state == State::CONNECTING) {
        NET4CXX_THROW_EXCEPTION(Exception, "cannot close a connection not yet connected");
    } else if (_state == State::OPEN) {
        ByteArray payload;
        if (code) {
            unsigned short closeCode = boost::endian::native_to_big(*code);
            payload.insert(payload.end(), (Byte *)&closeCode, (Byte *)&closeCode + sizeof(closeCode));
        }
        if (reason) {
            payload.insert(payload.end(), (Byte *)reason->data(), (Byte *)reason->data() + reason->size());
        }
        sendFrame(8u, payload.data(), payload.size());
        _state = State::CLOSING;
        _closedByMe = !isReply;

        _localCloseCode = code;
        _localCloseReason = std::move(reason);
        if (_closedByMe && _closeHandshakeTimeout > 0.0) {
            _closeHandshakeTimeoutCall = reactor()->callLater(_closeHandshakeTimeout,
                                                              [self = shared_from_this(), this](){
                onCloseHandshakeTimeout();
            });
        }
    } else {
        NET4CXX_ASSERT_MSG(false, "logic error");
    }
}

void WebSocketProtocol::sendFrame(Byte opcode, const Byte *payload, size_t length, bool fin, Byte rsv,
                                  boost::optional<WebSocketMask> mask, size_t payloadLen, size_t chopsize, bool sync) {
    ByteArray pl;
    size_t l;
    if (payloadLen > 0) {
        if (length == 0) {
            NET4CXX_THROW_EXCEPTION(Exception, "cannot construct repeated payload with length " +
                                               std::to_string(payloadLen) + " from payload of length 0");
        }
        l = payloadLen;
        for (size_t i = 0; i < payloadLen / length; ++i) {
            pl.insert(pl.end(), payload, payload + length);
        }
        pl.insert(pl.end(), payload, payload + payloadLen % length);
    } else {
        l = length;
        pl.assign(payload, payload + length);
    }

    Byte b0 = 0u;
    if (fin) {
        b0 |= (1u << 7u);
    }
    b0 |= (rsv % 8u) << 4u;
    b0 |= opcode % 128u;

    Byte b1 = 0u;
    ByteArray mv;
    if (mask || (!_isServer && _maskClientFrames) || (_isServer && _maskServerFrames)) {
        b1 |= 1u << 7u;
        if (!mask) {
            mask.emplace();
            Random::randBytes(*mask);
            mv.insert(mv.end(), mask->begin(), mask->end());
        }

        if (l > 0 && _applyMask) {
            auto masker = createXorMasker(*mask, l);
            masker->process(pl.data(), pl.size());
        }
    }
    ByteArray el;
    if (l <= 125) {
        b1 |= (Byte)l;
    } else if (l <= 0xFFFFu) {
        b1 |= 126u;
        uint16_t len = boost::endian::native_to_big((uint16_t)l);
        el.insert(el.end(), (Byte *)&len, (Byte *)&len + sizeof(len));
    } else if (l <= 0x7FFFFFFFFFFFFFFFu) {
        b1 |= 127u;
        uint64_t len = boost::endian::native_to_big((uint64_t)l);
        el.insert(el.end(), (Byte *)&len, (Byte *)&len + sizeof(len));
    } else {
        NET4CXX_THROW_EXCEPTION(Exception, "invalid payload length");
    }

    ByteArray raw;
    raw.push_back(b0);
    raw.push_back(b1);
    raw.insert(raw.end(), el.begin(), el.end());
    raw.insert(raw.end(), mv.begin(), mv.end());
    raw.insert(raw.end(), pl.begin(), pl.end());

    if (opcode == 0u || opcode == 1u || opcode == 2u) {
        _trafficStats._outgoingWebSocketFrames += 1;
    }

    if (_logFrames) {
        FrameHeader frameHeader(opcode, fin, rsv, l, mask);
        logTxFrame(frameHeader, payload, length, payloadLen, chopsize, sync);
    }
    sendData(raw, sync, chopsize);
}

void WebSocketProtocol::sendData(const ByteArray &data, bool sync, size_t chopsize) {
    if (chopsize > 0) {
        size_t i = 0, j;
        size_t n = data.size();
        bool done = false;
        while (!done) {
            j = i + chopsize;
            if (j >= n) {
                done = true;
                j = n;
            }
            _sendQueue.emplace_back(std::make_pair(ByteArray(data.begin() + i, data.begin() + j), true));
            i += chopsize;
        }
        trigger();
    } else {
        if (sync || !_sendQueue.empty()) {
            _sendQueue.emplace_back(std::make_pair(data, sync));
            trigger();
        } else {
            write(data);
            if (_state == State::OPEN) {
                _trafficStats._outgoingOctetsWireLevel += data.size();
            } else if (_state == State::CONNECTING || _state == State::PROXY_CONNECTING) {
                _trafficStats._preopenOutgoingOctetsWireLevel += data.size();
            }

            if (_logOctets) {
                logTxOctets(data.data(), data.size(), false);
            }
        }
    }
}

void WebSocketProtocol::send() {
    if (!_sendQueue.empty()) {
        auto e = std::move(_sendQueue.front());
        _sendQueue.pop_front();

        if (_state != State::CLOSED) {
            write(e.first);

            if (_state == State::OPEN) {
                _trafficStats._outgoingOctetsWireLevel += e.first.size();
            } else if (_state == State::CONNECTING || _state == State::PROXY_CONNECTING) {
                _trafficStats._preopenOutgoingOctetsWireLevel += e.first.size();
            }

            if (_logOctets) {
                logTxOctets(e.first.data(), e.first.size(), e.second);
            }
        } else {
            NET4CXX_LOG_DEBUG(gGenLog, "skipped delayed write, since connection is closed");
        }

        reactor()->callLater(QUEUED_WRITE_DELAY, [self = shared_from_this(), this](){
            send();
        });
    } else {
        _triggered = false;
    }
}

bool WebSocketProtocol::onFrameBegin() {
    if (_currentFrame->_opcode > 7u) {
        _controlFrameData.clear();
    } else {
        if (!_insideMessage) {
            _insideMessage = true;

            if (_perMessageCompress && _currentFrame->_rsv == 4u) {
                _isMessageCompressed = true;
                _perMessageCompress->startDecompressMessage();
            } else {
                _isMessageCompressed = false;
            }

            if (_currentFrame->_opcode == MESSAGE_TYPE_TEXT && _utf8validateIncomming) {
                _utf8validator.reset();
                _utf8validateIncomingCurrentMessage = true;
                _utf8validateLast = std::make_tuple(true, true, 0, 0);
            } else {
                _utf8validateIncomingCurrentMessage = false;
            }

            if (_trackedTimings) {
                _trackedTimings->track("onMessageBegin");
            }
            onMessageBegin(_currentFrame->_opcode == MESSAGE_TYPE_BINARY);
        }
        onMessageFrameBegin(_currentFrame->_length);
    }
    return true;
}

void WebSocketProtocol::onMessageFrameBegin(uint64_t length) {
    _frameLength = length;
    _frameData.clear();
    _messageDataTotalLength += length;
    if (!_failedByMe) {
        if (0 < _maxMessagePayloadSize && _maxMessagePayloadSize < _messageDataTotalLength) {
            _wasMaxMessagePayloadSizeExceeded = true;
            failConnection(CLOSE_STATUS_CODE_MESSAGE_TOO_BIG, "message exceeds payload limit of " +
                                                              std::to_string(_maxMessagePayloadSize) + " octets");
        } else if (0 < _maxFramePayloadSize && _maxFramePayloadSize < length) {
            _wasMaxFramePayloadSizeExceeded = true;
            failConnection(CLOSE_STATUS_CODE_POLICY_VIOLATION, "frame exceeds payload limit of " +
                                                               std::to_string(_maxFramePayloadSize) + " octets");
        }
    }
}

bool WebSocketProtocol::onFrameData(ByteArray payload) {
    if (_currentFrame->_opcode > 7u) {
        ConcatBuffer(_controlFrameData, std::move(payload));
    } else {
        size_t compressedLen, uncompressedLen;
        if (_isMessageCompressed) {
            compressedLen = payload.size();
            NET4CXX_LOG_DEBUG(gGenLog, "RX compressed %llu octets", (uint64_t)compressedLen);
            payload = _perMessageCompress->decompressMessageData(payload);
            uncompressedLen = payload.size();
        } else {
            compressedLen = payload.size();
            uncompressedLen = compressedLen;
        }

        if (_state == State::OPEN) {
            _trafficStats._incomingOctetsWebSocketLevel += compressedLen;
            _trafficStats._incomingOctetsAppLevel += uncompressedLen;
        }

        if (_utf8validateIncomingCurrentMessage) {
            _utf8validateLast = _utf8validator.validate(payload);
            if (!std::get<0>(_utf8validateLast)) {
                if (invalidPayload("encountered invalid UTF-8 while processing text message at payload octet index " +
                                   std::to_string(std::get<3>(_utf8validateLast)))) {
                    return false;
                }
            }
        }
        onMessageFrameData(std::move(payload));
    }
    return true;
}

void WebSocketProtocol::onMessageFrameData(ByteArray payload) {
    if (!_failedByMe) {
        if (_websocketVersion == 0) {
            _messageDataTotalLength += payload.size();
            if (0 < _maxMessagePayloadSize && _maxMessagePayloadSize < _messageDataTotalLength) {
                _wasMaxMessagePayloadSizeExceeded = true;
                failConnection(CLOSE_STATUS_CODE_MESSAGE_TOO_BIG, "message exceeds payload limit of " +
                                                                  std::to_string(_maxMessagePayloadSize) + " octets");
            }
            ConcatBuffer(_messageData, std::move(payload));
        } else {
            ConcatBuffer(_frameData, std::move(payload));
        }
    }
}

bool WebSocketProtocol::onFrameEnd() {
    if (_currentFrame->_opcode > 7u) {
        if (_logFrames) {
            logRxFrame(*_currentFrame, _controlFrameData.data(), _controlFrameData.size());
        }
        processControlFrame();
    } else {
        if (_state == State::OPEN) {
            _trafficStats._incomingWebSocketFrames += 1;
        }
        if (_logFrames) {
            logRxFrame(*_currentFrame, _frameData.data(), _frameData.size());
        }
        onMessageFrameEnd();
        if (_currentFrame->_fin) {
            if (_isMessageCompressed) {
                _perMessageCompress->endDecompressMessage();
            }

            if (_utf8validateIncomingCurrentMessage) {
                if (!std::get<1>(_utf8validateLast)) {
                    if (invalidPayload("UTF-8 text message payload ended within Unicode code point at payload octet "
                                       "index " + std::to_string(std::get<3>(_utf8validateLast)))) {
                        return false;
                    }
                }
            }

            if (_state == State::OPEN) {
                _trafficStats._incomingWebSocketMessages += 1;
            }

            onMessageEnd();
            _insideMessage = false;
        }

    }
    _currentFrame = boost::none;
    return true;
}

void WebSocketProtocol::onMessageEnd() {
    if (!_failedByMe) {
        if (_trackedTimings) {
            _trackedTimings->track("onMessage");
        }
        onMessage(std::move(_messageData), _messageIsBinary);
    }
    _messageData.clear();
}

bool WebSocketProtocol::processControlFrame() {
    ByteArray payload = std::move(_controlFrameData);
    if (_currentFrame->_opcode == 0) {
        boost::optional<unsigned short> code;
        boost::optional<std::string> reasonRaw;
        auto ll = payload.size();
        if (ll > 1) {
            code = boost::endian::big_to_native(*(uint16_t *)payload.data());
            if (ll > 2) {
                reasonRaw.emplace((const char *)payload.data() + 2, payload.size() - 2);
            }
        }
        if (onCloseFrame(code, std::move(reasonRaw))) {
            return false;
        }
    } else if (_currentFrame->_opcode == 9u) {
        onPing(std::move(payload));
    } else if (_currentFrame->_opcode == 10u) {
        if (!_autoPingPending.empty()) {
            try {
                if (payload == _autoPingPending) {
                    NET4CXX_LOG_DEBUG(gGenLog, "Auto ping/pong: received pending pong for auto-ping/pong");

                    if (!_autoPingTimeoutCall.cancelled()) {
                        _autoPingTimeoutCall.cancel();
                    }

                    _autoPingPending.clear();

                    if (_autoPingInterval != 0.0) {
                        _autoPingPendingCall = reactor()->callLater(_autoPingInterval,
                                                                    [self = shared_from_this(), this](){
                            sendAutoPing();
                        });
                    }
                } else {
                    NET4CXX_LOG_DEBUG(gGenLog, "Auto ping/pong: received non-pending pong");
                }
            } catch (std::exception &e) {
                NET4CXX_LOG_DEBUG(gGenLog, "Auto ping/pong: received non-pending pong");
            }
        }

        onPong(std::move(payload));
    } else {

    }
    return true;
}

bool WebSocketProtocol::onCloseFrame(boost::optional<unsigned short> code, boost::optional<std::string> reasonRaw) {
    _remoteCloseCode = boost::none;
    _remoteCloseReason = boost::none;

    if (code && ((*code < 1000u ||
                 (1000u <= *code && *code <= 2999u &&
                  !std::binary_search(CLOSE_STATUS_CODES_ALLOWED.begin(), CLOSE_STATUS_CODES_ALLOWED.end(), *code)) ||
                 *code >= 5000u))) {
        if (protocolViolation("invalid close code " + std::to_string(*code))) {
            return true;
        } else {
            _remoteCloseCode = CLOSE_STATUS_CODE_NORMAL;
        }
    } else {
        _remoteCloseCode = code;
    }

    if (reasonRaw) {
        Utf8Validator u;
        auto val = u.validate(*reasonRaw);

        if (!(std::get<0>(val) && std::get<1>(val))) {
            if (invalidPayload("invalid close reason (non-UTF8 payload)")) {
                return true;
            }
        } else {
            _remoteCloseReason = std::move(reasonRaw);
        }
    }

    if (_state == State::CLOSING) {
        if (!_closeHandshakeTimeoutCall.cancelled()) {
            NET4CXX_LOG_DEBUG(gGenLog, "connection closed properly: canceling closing handshake timeout");
            _closeHandshakeTimeoutCall.cancel();
        }

        _wasClean = true;

        if (_isMessageCompressed) {
            dropConnection(true);
        } else {
            if (_serverConnectionDropTimeout > 0.0) {
                _serverConnectionDropTimeoutCall = reactor()->callLater(_serverConnectionDropTimeout,
                                                                        [self = shared_from_this(), this](){
                    onServerConnectionDropTimeout();
                });
            }
        }
    } else if (_state == State::OPEN) {
        _wasClean = true;

        if (_websocketVersion == 0) {
            sendCloseFrame(boost::none, boost::none, true);
        } else {
            if (_echoCloseCodeReason) {
                sendCloseFrame(_remoteCloseCode, WebSocketUtil::truncate(_remoteCloseReason, 123), true);
            } else {
                sendCloseFrame(CLOSE_STATUS_CODE_NORMAL, boost::none, true);
            }
        }

        if (_isServer) {
            dropConnection(false);
        }
    } else if (_state == State::CLOSED) {
        _wasClean = false;
    } else {
        NET4CXX_THROW_EXCEPTION(Exception, "logic error");
    }
    return false;
}

void WebSocketProtocol::sendAutoPing() {
    NET4CXX_LOG_DEBUG(gGenLog, "Auto ping/pong: sending ping auto-ping/pong");

    _autoPingPendingCall.reset();
    _autoPingPending = WebSocketUtil::newid(_autoPingSize);

    sendPing(_autoPingPending);

    if (_autoPingTimeout != 0.0) {
        NET4CXX_LOG_DEBUG(gGenLog, "Expecting ping in %lf seconds for auto-ping/pong", _autoPingTimeout);
        _autoPingTimeoutCall = reactor()->callLater(_autoPingTimeout, [self = shared_from_this(), this](){
            onAutoPingTimeout();
        });
    }
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