//
// Created by yuwenyong on 18-1-11.
//

#include "net4cxx/plugins/websocket/protocol.h"
#include <boost/endian/conversion.hpp>
#include "net4cxx/common/crypto/base64.h"
#include "net4cxx/common/crypto/hashlib.h"
#include "net4cxx/common/utilities/random.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN

const std::vector<int> WebSocketProtocol::SUPPORTED_SPEC_VERSIONS = {10, 11, 12, 13, 14, 15, 16, 17, 18};

const std::vector<int> WebSocketProtocol::SUPPORTED_PROTOCOL_VERSIONS = {8, 13};

const std::map<int, int> WebSocketProtocol::SPEC_TO_PROTOCOL_VERSION = {
        {10, 8},
        {11, 8},
        {12, 8},
        {13, 13},
        {14, 13},
        {15, 13},
        {16, 13},
        {17, 13},
        {18, 13}
};

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

const std::string WebSocketProtocol::WS_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

const double WebSocketProtocol::QUEUED_WRITE_DELAY = 0.00001;

void WebSocketProtocol::onOpen() {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onOpen");
}

void WebSocketProtocol::onMessage(ByteArray payload, bool isBinary) {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onMessage(payload=<%llu bytes)>, isBinary=%s",
                      payload.size(), isBinary ? "true" : "false");
}

void WebSocketProtocol::onPing(ByteArray payload) {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onPing(payload=<%llu bytes>)", payload.size());
    if (_state == State::OPEN) {
        sendPong(payload.data(), payload.size());
    }
}

void WebSocketProtocol::onPong(ByteArray payload) {
    NET4CXX_LOG_DEBUG(gGenLog, "WebSocketProtocol.onPong(payload=<%llu bytes>)", payload.size());
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

     Byte opcode = isBinary ? 2 : 1;
     bool sendCompressed;
     ByteArray payload1;

    _trafficStats._outgoingWebSocketMessages += 1;

    if (_perMessageCompress && !doNotCompress) {
        sendCompressed = true;
        _perMessageCompress->startCompressMessage();
        _trafficStats._outgoingOctetsAppLevel += length;

        payload1 = _perMessageCompress->compressMessageData(payload, length);
        auto payload2 = _perMessageCompress->endCompressMessage();
        ConcatBuffer(payload1, std::move(payload2));

        payload = payload1.data();
        length = payload1.size();

        _trafficStats._outgoingOctetsWebSocketLevel += length;
    } else {
        sendCompressed = false;
        _trafficStats._outgoingOctetsAppLevel += length;
        _trafficStats._outgoingOctetsWebSocketLevel += length;
    }

    size_t pfs = 0;
    if (fragmentSize) {
        pfs = fragmentSize;
    } else {
        if (_autoFragmentSize > 0) {
            pfs = _autoFragmentSize;
        }
    }

    if (pfs == 0 || length <= pfs) {
        sendFrame(opcode, payload, length, true, (Byte)(sendCompressed ? 4 : 0), {}, 0, 0, sync);
    } else {
        size_t i = 0, j = 0;
        bool done = false, first = true;
        while (!done) {
            j = i + pfs;
            if (j > length) {
                done = true;
                j = length;
            }
            if (first) {
                sendFrame(opcode, payload + i, j - i, done, (Byte)(sendCompressed ? 4 : 0), {}, 0, 0, sync);
                first = false;
            } else {
                sendFrame(0, payload + i, j - i, done, 0u, {}, 0, 0, sync);
            }
            i += pfs;
        }
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
    _peer = makePeerName();

    setTrackTimings(_trackTimings);
    _sendState = SendState::GROUND;

    if (_openHandshakeTimeout > 0.0) {
        _openHandshakeTimeoutCall = reactor()->callLater(_openHandshakeTimeout, [self = shared_from_this(), this](){
            onOpenHandshakeTimeout();
        });
    }

    NET4CXX_LOG_DEBUG(gGenLog, "Connection made to %s", _peer.c_str());
    setNoDelay(_tcpNoDelay);
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
        _serverConnectionDropTimeoutCall.reset();
    }

    if (!_autoPingPendingCall.cancelled()) {
        NET4CXX_LOG_DEBUG(gGenLog,"Auto ping/pong: canceling autoPingPendingCall upon lost connection");
        _autoPingPendingCall.cancel();
        _autoPingPendingCall.reset();
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
            onClose(_wasClean, _remoteCloseCode, _remoteCloseReason);
        }
    }
}

std::string WebSocketProtocol::makePeerName() const {
    std::string res;
    auto address = getRemoteAddress();
    if (NetUtil::isValidIPv4(address)) {
        res = "tcp4:" + address + ":" + std::to_string(getRemotePort());
    } else if (NetUtil::isValidIPv6(address)) {
        res = "tcp6:" + address + ":" + std::to_string(getRemotePort());
    } else if (!address.empty()) {
        res = "unix:" + address;
    } else {
        res = "?:";
    }
    return res;
}

void WebSocketProtocol::setTrackTimings(bool enable) {
    _trackTimings = enable;
    if (_trackTimings) {
        if (!_trackedTimings) {
            _trackedTimings = Timings{};
        }
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
        NET4CXX_LOG_DEBUG(gGenLog, "received data in State::CLOSED");
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
                                                          _perMessageCompress->getExtensionName()))) {
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
                                                          _perMessageCompress->getExtensionName()))) {
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
                NET4CXX_ASSERT(framePayloadLen1 == 127u);
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
    sendData(raw.data(), raw.size(), sync, chopsize);
}

void WebSocketProtocol::sendData(const Byte *data, size_t length, bool sync, size_t chopsize) {
    if (chopsize > 0) {
        size_t i = 0, j;
        size_t n = length;
        bool done = false;
        while (!done) {
            j = i + chopsize;
            if (j >= n) {
                done = true;
                j = n;
            }
            _sendQueue.emplace_back(std::make_pair(ByteArray(data + i, data + j), true));
            i += chopsize;
        }
        trigger();
    } else {
        if (sync || !_sendQueue.empty()) {
            _sendQueue.emplace_back(std::make_pair(ByteArray(data, data + length), sync));
            trigger();
        } else {
            write(data, length);
            if (_state == State::OPEN) {
                _trafficStats._outgoingOctetsWireLevel += length;
            } else if (_state == State::CONNECTING || _state == State::PROXY_CONNECTING) {
                _trafficStats._preopenOutgoingOctetsWireLevel += length;
            }

            if (_logOctets) {
                logTxOctets(data, length, false);
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
            NET4CXX_LOG_DEBUG(gGenLog, "RX compressed %llu octets", compressedLen);
            payload = _perMessageCompress->decompressMessageData(payload.data(), payload.size());
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
        if (_webSocketVersion == 0) {
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
    if (_currentFrame->_opcode == 8u) {
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
                    _autoPingTimeoutCall.reset();

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
            _closeHandshakeTimeoutCall.reset();
        }

        _wasClean = true;

        if (_isServer) {
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

        if (_webSocketVersion == 0) {
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

WebSocketExtensionList WebSocketProtocol::parseExtensionsHeader(const std::string &header, bool removeQuotes) {
    WebSocketExtensionList extensions;
    StringVector exts, ext, p;
    std::string extension, key;
    WebSocketExtensionParams params;
    boost::optional<std::string> value;

    exts = StrUtil::split(header, ',');
    for (auto &e: exts) {
        boost::trim(e);
        if (!e.empty()) {
            ext = StrUtil::split(e, ';');
            if (!ext.empty()) {
                boost::trim(ext[0]);
                extension = boost::to_lower_copy(ext[0]);
                params.clear();
                for (size_t i = 1; i != ext.size(); ++i) {
                    boost::trim(ext[i]);
                    p = StrUtil::split(ext[i], '=');
                    for (auto &x: p) {
                        boost::trim(x);
                    }
                    key = boost::to_lower_copy(p[0]);
                    if (p.size() > 1) {
                        p.erase(p.begin());
                        value = boost::join(p, "=");
                        if (removeQuotes) {
                            if (!value->empty() && value->front() == '"') {
                                value->erase(value->begin());
                            }
                            if (!value->empty() && value->back() == '"') {
                                value->pop_back();
                            }
                        }
                    } else {
                        value = boost::none;
                    }
                    if (params.find(key) == params.end()) {
                        params[key] = {};
                    }
                    params[key].emplace_back(std::move(value));
                }
                extensions.emplace_back(std::make_pair(std::move(extension), std::move(params)));
            } else {
                NET4CXX_ASSERT(false);
            }
        }
    }
    return extensions;
}


const char* WebSocketServerProtocol::SERVER_STATUS_TEMPLATE = R"(<!DOCTYPE html>
<html>
    <head>
        %s
    </head>
    <body>
        <p>
            I am not Web server, but a <b>WebSocket Endpoint</b>.
        </p>
    </body>
</html>
)";

std::pair<std::string, WebSocketHeaders> WebSocketServerProtocol::onConnect(ConnectionRequest request) {
    return std::make_pair("", WebSocketHeaders{});
}

void WebSocketServerProtocol::connectionMade() {
    auto factory = getFactory<WebSocketServerFactory>();
    _isServer = true;
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
    _perMessageCompressionAccept = factory->getPerMessageCompressionAccept();
    _serverFlashSocketPolicy = factory->getServerFlashSocketPolicy();
    _flashSocketPolicy = factory->getFlashSocketPolicy();
    _allowedOrigins = factory->getAllowedOrigins();
    _allowedOriginsPatterns = factory->getAllowedOriginsPatterns();
    _allowNullOrigin = factory->getAllowNullOrigin();
    _maxConnections = factory->getMaxConnections();
    _trustXForwardedFor = factory->getTrustXForwardedFor();

    _state = State::CONNECTING;
    BaseType::connectionMade();
    factory->incConnectionCount();
    NET4CXX_LOG_DEBUG(gGenLog, "connection accepted from peer %s", _peer);
}

void WebSocketServerProtocol::connectionLost(std::exception_ptr reason) {
    BaseType::connectionLost(reason);
    getFactory<WebSocketServerFactory>()->decConnectionCount();
}

void WebSocketServerProtocol::processProxyConnect() {
    NET4CXX_THROW_EXCEPTION(Exception, "This isn't a proxy server");
}

void WebSocketServerProtocol::processHandshake() {
    auto factory = getFactory<WebSocketServerFactory>();

    const char *endOfHeader = StrNStr((const char *)_data.data(), _data.size(), "\x0d\x0a\x0d\x0a");
    if (endOfHeader != nullptr) {
        _httpRequestData.assign((const char*)_data.data(), endOfHeader + 4);
        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP request:\n\n%s\n\n", _httpRequestData);

        std::map<std::string, int> httpHeadersCnt;
        try {
            std::tie(_httpStatusLine, _httpHeaders, httpHeadersCnt) = WebSocketUtil::parseHttpHeader(_httpRequestData);
        } catch (std::exception &e) {
            failHandshake(StrUtil::format("Error during parsing of HTTP status line / request headers : %s", e.what()));
            return;
        }

        auto xForwardedFor = _httpHeaders.find("x-forwarded-for");
        if (xForwardedFor != _httpHeaders.end() && _trustXForwardedFor != 0) {
            auto addresses = StrUtil::split(xForwardedFor->second, ',');
            if (addresses.size() > _trustXForwardedFor) {
                _peer = boost::trim_copy(addresses[addresses.size() - _trustXForwardedFor]);
            } else {
                _peer = boost::trim_copy(addresses.front());
            }
        }

        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP status line in opening handshake : %s", _httpStatusLine);
        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP headers in opening handshake : %s", StringMapToString(_httpHeaders));

        auto rl = StrUtil::split(_httpStatusLine);
        if (rl.size() != 3) {
            failHandshake(StrUtil::format("Bad HTTP request status line '%s'", _httpStatusLine));
            return;
        }
        if (boost::trim_copy(rl[0]) != "GET") {
            failHandshake(StrUtil::format("HTTP method '%s' not allowed", rl[0]), 405);
            return;
        }
        auto vs = StrUtil::split(boost::trim_copy(rl[2]), '/');
        if (vs.size() != 2 || vs[0] != "HTTP" || vs[1] != "1.1") {
            failHandshake(StrUtil::format("Unsupported HTTP version '%s'", rl[2]), 505);
            return;
        }

        _httpRequestUri = boost::trim_copy(rl[1]);
        try {
            std::string scheme, netloc, path, query, fragment;
            std::tie(scheme, netloc, path, std::ignore, query, fragment) =
                    (const URLParseResultBase &)URLParse::urlParse(_httpRequestUri);

            if (scheme != "" or netloc != "") {

            }

            if (fragment != "") {
                failHandshake(StrUtil::format("HTTP requested resource contains a fragment identifier '%s'", fragment));
                return;
            }

            _httpRequestPath = path;
            _httpRequestParams = URLParse::parseQS(query);
        } catch (...) {
            failHandshake(StrUtil::format("Bad HTTP request resource - could not parse '%s'", _httpRequestUri));
            return;
        }

        auto hostIter = _httpHeaders.find("host");
        if (hostIter == _httpHeaders.end()) {
            failHandshake("HTTP Host header missing in opening handshake request");
            return;
        }

        if (httpHeadersCnt["host"] > 1) {
            failHandshake("HTTP Host header appears more than once in opening handshake request");
            return;
        }

        _httpRequestHost = boost::trim_copy(hostIter->second);

        if (_httpRequestHost.find(':') != std::string::npos && !boost::ends_with(_httpRequestHost, "]")) {
            auto pos = _httpRequestHost.rfind(':');
            std::string h(_httpRequestHost.begin(), std::next(_httpRequestHost.begin(), pos));
            std::string p(next(_httpRequestHost.begin(), pos + 1), _httpRequestHost.end());
            unsigned short port;
            try {
                boost::trim(p);
                port = (unsigned short)std::stoul(p);
            } catch (...) {
                failHandshake(StrUtil::format("invalid port '%s' in HTTP Host header '%s'", p, _httpRequestHost));
                return;
            }

            unsigned short externalPort = factory->getExternalPort();
            if (externalPort) {
                if (port != externalPort) {
                    failHandshake(StrUtil::format("port %u in HTTP Host header '%s' "
                                                  "does not match server listening port %u",
                                                  port, _httpRequestHost, externalPort));
                    return;
                }
            } else {
                NET4CXX_LOG_DEBUG(gGenLog, "skipping opening handshake port checking - "
                                           "neither WS URL nor external port set");
            }

            _httpRequestHost = std::move(h);
        } else {
            unsigned short externalPort = factory->getExternalPort();
            bool isSecure = factory->isSecure();
            if (externalPort) {
                if (!((isSecure && externalPort == 443) || (!isSecure && externalPort == 80))) {
                    failHandshake(StrUtil::format("missing port in HTTP Host header '%s' and "
                                                  "server runs on non-standard port %u (wss = %s)",
                                                  _httpRequestHost, externalPort, isSecure ? "true" : "false"));
                }
            } else {
                NET4CXX_LOG_DEBUG(gGenLog, "skipping opening handshake port checking - "
                                           "neither WS URL nor external port set");
            }
        }

        auto upgradeIter = _httpHeaders.find("upgrade");
        if (upgradeIter == _httpHeaders.end()) {
            if (_webStatus) {
                auto redirectIter = _httpRequestParams.find("redirect");
                if (redirectIter != _httpRequestParams.end() && !redirectIter->second.empty()) {
                    auto &url = redirectIter->second.front();
                    auto afterIter = _httpRequestParams.find("after");
                    if (afterIter != _httpRequestParams.end() && !afterIter->second.empty()) {
                        int after = std::stoi(afterIter->second.front());
                        NET4CXX_LOG_DEBUG(gGenLog, "HTTP Upgrade header missing : render server status page and "
                                                   "meta-refresh-redirecting to %s after %d seconds", url, after);
                        sendServerStatus(url, after);
                    } else {
                        NET4CXX_LOG_DEBUG(gGenLog, "HTTP Upgrade header missing : 303-redirecting to %s", url);
                        sendRedirect(url);
                    }
                } else {
                    NET4CXX_LOG_DEBUG(gGenLog, "HTTP Upgrade header missing : render server status page");
                    sendServerStatus();
                }
                dropConnection(false);
                return;
            } else {
                failHandshake("HTTP Upgrade header missing", 426);
                return;
            }
        }
        bool upgradeWebSocket = false;
        for (auto &u: StrUtil::split(upgradeIter->second, ',')) {
            if (boost::to_lower_copy(boost::trim_copy(u)) == "websocket") {
                upgradeWebSocket = true;
                break;
            }
        }
        if (!upgradeWebSocket) {
            failHandshake(StrUtil::format("HTTP Upgrade headers do not include "
                                          "'websocket' value (case-insensitive) : %s", upgradeIter->second));
            return;
        }

        auto connectionIter = _httpHeaders.find("connection");
        if (connectionIter == _httpHeaders.end()) {
            failHandshake("HTTP Connection header missing");
            return;
        }
        bool connectionUpgrade = false;
        for (auto &c: StrUtil::split(connectionIter->second, ',')) {
            if (boost::to_lower_copy(boost::trim_copy(c)) == "upgrade") {
                connectionUpgrade = true;
                break;
            }
        }
        if (!connectionUpgrade) {
            failHandshake(StrUtil::format("HTTP Connection headers do not include "
                                          "'upgrade' value (case-insensitive) : %s", connectionIter->second));
            return;
        }

        int version = -1;
        auto webSocketVersionIter = _httpHeaders.find("sec-websocket-version");
        if (webSocketVersionIter == _httpHeaders.end()) {
            NET4CXX_LOG_DEBUG(gGenLog, "Hixie76 protocol detected");
            failHandshake("WebSocket connection denied - Hixie76 protocol not supported.");
            return;
        } else {
            NET4CXX_LOG_DEBUG(gGenLog, "Hybi protocol detected");
            if (httpHeadersCnt["sec-websocket-version"] > 1) {
                failHandshake("HTTP Sec-WebSocket-Version header appears more than once in opening handshake request");
                return;
            }
            try {
                version = std::stoi(webSocketVersionIter->second);
            } catch (...) {
                failHandshake(StrUtil::format("could not parse HTTP Sec-WebSocket-Version header "
                                              "'%s' in opening handshake request", webSocketVersionIter->second));
                return;
            }
        }

        if (std::find(_versions.begin(), _versions.end(), version) == _versions.end()) {
            auto sv = _versions;
            std::sort(sv.begin(), sv.end(), std::greater<>());
            std::string svs;
            bool first = true;
            for (auto x: sv) {
                if (first) {
                    first = false;
                } else {
                    svs += ',';
                }
                svs += std::to_string(x);
            }
            failHandshake(StrUtil::format("WebSocket version %d not supported (supported versions: %s)", version, svs),
                          400, {{"Sec-WebSocket-Version", svs}});
            return;
        } else {
            _webSocketVersion = version;
        }

        auto webSocketProtocolIter = _httpHeaders.find("sec-websocket-protocol");
        if (webSocketProtocolIter != _httpHeaders.end()) {
            auto protocols = StrUtil::split(webSocketProtocolIter->second, ',');
            std::map<std::string, int> pp;
            for (auto &p: protocols) {
                boost::trim(p);
                if (pp.find(p) != pp.end()) {
                    failHandshake(StrUtil::format("duplicate protocol '%s' specified in "
                                                  "HTTP Sec-WebSocket-Protocol header", p));
                    return;
                } else {
                    pp[p] = 1;
                }
            }
            _webSocketProtocols = std::move(protocols);
        }

        std::string webSocketOriginHeaderKey;
        if (_webSocketVersion < 13) {
            webSocketOriginHeaderKey = "sec-websocket-origin";
        } else {
            webSocketOriginHeaderKey = "origin";
        }

        bool haveOrigin = false;
        WebSocketUtil::UrlToOriginResult originTuple;
        auto webSocketOriginIter = _httpHeaders.find(webSocketOriginHeaderKey);
        if (webSocketOriginIter != _httpHeaders.end()) {
            if (httpHeadersCnt[webSocketOriginHeaderKey] > 1) {
                failHandshake("HTTP Origin header appears more than once in opening handshake request");
                return;
            }
            _webSocketOrigin = boost::trim_copy(webSocketOriginIter->second);
            try {
                originTuple = WebSocketUtil::urlToOrigin(_webSocketOrigin);
            } catch (ValueError &e) {
                failHandshake(StrUtil::format("HTTP Origin header invalid: %s", e.what()));
                return;
            }
            haveOrigin = true;
        }

        if (haveOrigin) {
            bool  originIsAllowed;
            if (std::get<0>(originTuple) == "null" && _allowNullOrigin) {
                originIsAllowed = true;
            } else {
                originIsAllowed = WebSocketUtil::isSameOrigin(originTuple, factory->isSecure() ? "https" : "http",
                                                              factory->getExternalPort() != 0 ?
                                                              factory->getExternalPort() : factory->getPort(),
                                                              _allowedOriginsPatterns);
            }
            if (!originIsAllowed) {
                failHandshake(StrUtil::format("WebSocket connection denied: origin '%s' not allowed",
                                              _webSocketOrigin));
                return;
            }
        }

        auto webSocketKeyIter = _httpHeaders.find("sec-websocket-key");
        if (webSocketKeyIter == _httpHeaders.end()) {
            failHandshake("HTTP Sec-WebSocket-Key header missing");
            return;
        }
        if (httpHeadersCnt["sec-websocket-key"] > 1) {
            failHandshake("HTTP Sec-WebSocket-Key header appears more than once in opening handshake request");
            return;
        }
        auto key = boost::trim_copy(webSocketKeyIter->second);
        if (key.length() != 24) {
            failHandshake(StrUtil::format("bad Sec-WebSocket-Key (length must be 24 ASCII chars) '%s'", key));
            return;
        }
        if (!boost::ends_with(key, "==")) {
            failHandshake(StrUtil::format("bad Sec-WebSocket-Key (invalid base64 encoding) '%s'", key));
            return;
        }
        for (size_t i = 0; i != key.length() - 2; ++i) {
            auto c = key[i];
            if (!(('a' <= c && c <= 'z') ||
                  ('A' <= c && c <= 'Z') ||
                  ('0' <= c && c <= '9') ||
                  c == '+' ||
                  c == '/')) {
                failHandshake(StrUtil::format("bad character '%c' in Sec-WebSocket-Key (invalid base64 encoding) '%s'",
                                              c, key));
                return;
            }
        }

        auto webSocketExtensionsIter = _httpHeaders.find("sec-websocket-extensions");
        if (webSocketExtensionsIter != _httpHeaders.end()) {
            if (httpHeadersCnt["sec-websocket-extensions"] > 1) {
                failHandshake("HTTP Sec-WebSocket-Extensions header appears more than once "
                              "in opening handshake request");
                return;
            } else {
                _webSocketExtensions = parseExtensionsHeader(webSocketExtensionsIter->second);
            }
        }

        _data.erase(_data.begin(), std::next(_data.begin(), ((const Byte *)endOfHeader + 4 - _data.data())));
        _webSocketKey = std::move(key);

        if (_maxConnections > 0 && factory->getConnectionCount() > _maxConnections) {
            failHandshake("maximum number of connections reached", 503);
        } else {
            try {
                auto res = onConnect(ConnectionRequest(_peer, _httpHeaders, _httpRequestHost, _httpRequestPath,
                                                       _httpRequestParams, _webSocketVersion, _webSocketOrigin,
                                                       _webSocketProtocols, _webSocketExtensions));
                succeedHandshake(std::move(res));
            } catch (ConnectionDeny &e) {
                failHandshake(e.getReason(), e.getCode());
            } catch (std::exception &e) {
                NET4CXX_LOG_WARN(gGenLog, "Unexpected exception in onConnect ['%s']", e.what());
                failHandshake(StrUtil::format("Internal server error: %s", e.what()),
                              ConnectionDeny::INTERNAL_SERVER_ERROR);
            }
        }
    } else if (_serverFlashSocketPolicy) {
        const char policyFileRequest[] = {'<', 'p', 'o', 'l', 'i', 'c', 'y',
                                          '-', 'f', 'i', 'l', 'e',
                                          '-', 'r', 'e', 'q', 'u', 'e', 's', 't',
                                          '/', '>', '\x00'};
        const char *flashPolicyFileRequest = StrNStr((const char *)_data.data(), _data.size(), policyFileRequest,
                                                     sizeof(policyFileRequest));
        if (flashPolicyFileRequest) {
            NET4CXX_LOG_DEBUG(gGenLog, "received Flash Socket Policy File request");

            if (_serverFlashSocketPolicy) {
                NET4CXX_LOG_DEBUG(gGenLog, "sending Flash Socket Policy File : %s", _flashSocketPolicy);
                sendData((const Byte *)_flashSocketPolicy.data(), _flashSocketPolicy.size());
                _wasServingFlashSocketPolicyFile = true;
                dropConnection();
            } else {
                NET4CXX_LOG_DEBUG(gGenLog, "No Flash Policy File served.");
            }
        }
    }
}

void WebSocketServerProtocol::succeedHandshake(std::pair<std::string, WebSocketHeaders> res) {
    auto factory = getFactory<WebSocketServerFactory>();

    std::string protocol;
    WebSocketHeaders headers;
    protocol = std::move(res.first);
    headers = std::move(res.second);

    if (!protocol.empty() &&
        std::find(_webSocketProtocols.begin(), _webSocketProtocols.end(), protocol) == _webSocketProtocols.end()) {
        NET4CXX_THROW_EXCEPTION(Exception, "protocol accepted must be from the list client sent or <empty>");
    }

    _webSocketProtocolInUse = std::move(protocol);

    StringVector extensionResponse;
    std::vector<PerMessageCompressOfferPtr> pmceOffers;

    for (auto &webSocketExtension: _webSocketExtensions) {
        auto &extension = webSocketExtension.first;
        auto &params = webSocketExtension.second;
        NET4CXX_LOG_DEBUG(gGenLog, "parsed WebSocket extension '%s'", extension);
        auto PMCE = PERMESSAGE_COMPRESSION_EXTENSION.find(extension);
        if (PMCE != PERMESSAGE_COMPRESSION_EXTENSION.end()) {
            try {
                auto offer = PMCE->second->createOfferFromParams(params);
                pmceOffers.emplace_back(std::move(offer));
            } catch (std::exception &e) {
                failHandshake(e.what());
                return;
            }
        } else {
            NET4CXX_LOG_DEBUG(gGenLog, "client requested '%s' extension we don't support or which is not activated",
                              extension);
        }
    }

    if (!pmceOffers.empty()) {
        PerMessageCompressOfferAcceptPtr accept;
        if (_perMessageCompressionAccept) {
            accept = _perMessageCompressionAccept(std::move(pmceOffers));
        }

        if (accept) {
            auto PMCE = PERMESSAGE_COMPRESSION_EXTENSION.find(accept->getExtensionName());
            _perMessageCompress = PMCE->second->createFromOfferAccept(true, accept);
            _webSocketExtensionsInUse.emplace_back(_perMessageCompress);
            extensionResponse.emplace_back(accept->getExtensionString());
        } else {
            NET4CXX_LOG_DEBUG(gGenLog, "client request permessage-compress extension, "
                                       "but we did not accept any offers");
        }
    }

    std::string response = "HTTP/1.1 101 Switching Protocols\x0d\x0a";

    if (!factory->getServer().empty()) {
        response += StrUtil::format("Server: %s\x0d\x0a", factory->getServer());
    }

    response += "Upgrade: WebSocket\x0d\x0a";
    response += "Connection: Upgrade\x0d\x0a";

    for (auto &uh: factory->getHeaders()) {
        for (auto &headerValue: uh.second) {
            response += StrUtil::format("%s: %s\x0d\x0a", uh.first, headerValue);
        }
    }

    for (auto &uh: headers) {
        for (auto &headerValue: uh.second) {
            response += StrUtil::format("%s: %s\x0d\x0a", uh.first, headerValue);
        }
    }

    if (!_webSocketProtocolInUse.empty()) {
        response += StrUtil::format("Sec-WebSocket-Protocol: %s\x0d\x0a", _webSocketProtocolInUse);
    }

    SHA1Object sha1;
    sha1.update(_webSocketKey + WS_MAGIC);
    auto secWebSocketAccept = Base64::b64encode(sha1.digest());

    response += StrUtil::format("Sec-WebSocket-Accept: %s\x0d\x0a", secWebSocketAccept);

    if (!extensionResponse.empty()) {
        response += StrUtil::format("Sec-WebSocket-Extensions: %s\x0d\x0a", boost::join(extensionResponse, ", "));
    }

    response += "\x0d\x0a";

    NET4CXX_LOG_DEBUG(gGenLog, "sending HTTP response:\n\n%s", response);
    sendData((const Byte *)response.data(), response.size());

    _httpResponseData = std::move(response);
    _state = State::OPEN;

    if (!_openHandshakeTimeoutCall.cancelled()) {
        NET4CXX_LOG_DEBUG(gGenLog, "cancel openHandshakeTimeoutCall");
        _openHandshakeTimeoutCall.cancel();
    }

    _insideMessage = false;
    _currentFrame = boost::none;

    if (_autoPingInterval > 0.0) {
        _autoPingPendingCall = reactor()->callLater(_autoPingInterval, [self = shared_from_this(), this](){
            sendAutoPing();
        });
    }

    if (_trackedTimings) {
        _trackedTimings->track("onOpen");
    }
    onOpen();

    if (!_data.empty()) {
        consumeData();
    }
}

void WebSocketServerProtocol::failHandshake(const std::string &reason, int code, StringMap responseHeaders) {
    _wasNotCleanReason = reason;
    NET4CXX_LOG_INFO(gGenLog, "failing WebSocket opening handshake ('%s')", reason);
    sendHttpErrorResponse(code, reason, std::move(responseHeaders));
    dropConnection(false);
}

void WebSocketServerProtocol::sendHttpErrorResponse(int code, const std::string &reason, StringMap responseHeaders) {
    std::string response = StrUtil::format("HTTP/1.1 %d %s\x0d\x0a", code, reason);
    for (auto &h: responseHeaders) {
        response += StrUtil::format("%s: %s\x0d\x0a", h.first, h.second);
    }
    response += "\x0d\x0a";
    sendData((const Byte *)response.data(), response.size());
}

void WebSocketServerProtocol::sendHtml(const std::string &html) {
    std::string response = "HTTP/1.1 200 OK\x0d\x0a";
    std::string server = getFactory<WebSocketServerFactory>()->getServer();
    if (!server.empty()) {
        response += StrUtil::format("Server: %s\x0d\x0a", server);
    }
    response += "Content-Type: text/html; charset=UTF-8\x0d\x0a";
    response += StrUtil::format("Content-Length: %llu\x0d\x0a", html.size());
    response += "\x0d\x0a";
    sendData((const Byte *)response.data(), response.size());
    sendData((const Byte *)html.data(), html.size());
}

void WebSocketServerProtocol::sendRedirect(const std::string &url) {
    std::string response = "HTTP/1.1 303 OK\x0d\x0a";
    std::string server = getFactory<WebSocketServerFactory>()->getServer();
    if (!server.empty()) {
        response += StrUtil::format("Server: %s\x0d\x0a", server);
    }
    response += StrUtil::format("Location: %s\x0d\x0a", url);
    response += "\x0d\x0a";
    sendData((const Byte *)response.data(), response.size());
}

void WebSocketServerProtocol::sendServerStatus(const std::string &redirectUrl, int redirectAfter) {
    std::string redirect;
    if (!redirectUrl.empty()) {
        redirect = StrUtil::format(R"(<meta http-equiv="refresh" content="%d;URL='%s'">)", redirectAfter, redirectUrl);
    }
    sendHtml(StrUtil::format(SERVER_STATUS_TEMPLATE, redirect));
}



void WebSocketServerFactory::setSessionParameters(std::string url, StringVector protocols, std::string server,
                                                  WebSocketHeaders headers, unsigned short externalPort) {
    if (server.empty()) {
        server = NET4CXX_VER;
    }
    bool isSecure;
    std::string host, resource, path;
    unsigned short port;
    QueryArgListMap params;
    std::tie(isSecure, host, port, resource, path, params) = WebSocketUtil::parseUrl(
            url.empty() ? "ws://localhost" : url);
    if (!params.empty()) {
        NET4CXX_THROW_EXCEPTION(Exception, "query parameters specified for server WebSocket URL");
    }
    _url = std::move(url);
    _isSecure = isSecure;
    _host = std::move(host);
    _port = port;
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


void WebSocketClientProtocol::onConnect(ConnectionResponse response) {

}

void WebSocketClientProtocol::connectionMade() {
    auto factory = getFactory<WebSocketClientFactory>();
    _isServer = false;
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
    _perMessageCompressionAccept = factory->getPerMessageCompressionAccept();
    _perMessageCompressionOffers = factory->getPerMessageCompressionOffers();

    if (factory->getProxy().empty()) {
        _state = State::CONNECTING;
    } else {
        _state = State::PROXY_CONNECTING;
    }

    BaseType::connectionMade();
    NET4CXX_LOG_DEBUG(gGenLog, "connection to %s established", _peer);

    if (!factory->getProxy().empty()) {
        startProxyConnect();
    } else {
        startHandshake();
    }
}

void WebSocketClientProtocol::connectionLost(std::exception_ptr reason) {
    BaseType::connectionLost(reason);
}

void WebSocketClientProtocol::processProxyConnect() {
    const char *endOfHeader = StrNStr((const char *)_data.data(), _data.size(), "\x0d\x0a\x0d\x0a");
    if (endOfHeader != nullptr) {
        std::string httpResponseData((const char*)_data.data(), endOfHeader + 4);
        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP response:\n\n%s\n\n", httpResponseData);

        std::string httpStatusLine;
        StringMap httpHeaders;
        std::map<std::string, int> httpHeadersCnt;
        std::tie(httpStatusLine, httpHeaders, httpHeadersCnt) = WebSocketUtil::parseHttpHeader(httpResponseData);

        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP status line for proxy connect request : %s", httpStatusLine);
        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP headers for proxy connect request : %s",
                          StringMapToString(httpHeaders));

        auto sl = StrUtil::split(httpStatusLine);
        if (sl.size() < 2) {
            failProxyConnect(StrUtil::format("Bad HTTP response status line '%s'", httpStatusLine));
            return;
        }

        auto httpVersion = boost::trim_copy(sl[0]);
        if (httpVersion != "HTTP/1.1" && httpVersion != "HTTP/1.0") {
            failProxyConnect(StrUtil::format("Unsupported HTTP version ('%s')", httpVersion));
            return;
        }

        int statusCode;
        try {
            statusCode = std::stoi(boost::trim_copy(sl[1]));
        } catch (...) {
            failProxyConnect(StrUtil::format("Bad HTTP status code ('%s')", boost::trim_copy(sl[1])));
            return;
        }

        if (!(200 <= statusCode && statusCode < 300)) {
            std::string reason;

            if (sl.size() > 2) {
                sl.erase(sl.begin(), std::next(sl.begin(), 2));
                reason = " - " + boost::join(sl, "");
            }
            failProxyConnect(StrUtil::format("HTTP proxy connect failed (%d%s)", statusCode, reason));
            return;
        }

        _data.erase(_data.begin(), std::next(_data.begin(), ((const Byte *)endOfHeader + 4 - _data.data())));

        _state = State::CONNECTING;

        if (!_data.empty()) {
            consumeData();
        }
        startHandshake();
    }
}

void WebSocketClientProtocol::processHandshake() {
    auto factory = getFactory<WebSocketClientFactory>();

    const char *endOfHeader = StrNStr((const char *)_data.data(), _data.size(), "\x0d\x0a\x0d\x0a");
    if (endOfHeader != nullptr) {
        _httpResponseData.assign((const char*)_data.data(), endOfHeader + 4);
        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP response:\n\n%s\n\n", _httpResponseData);

        std::map<std::string, int> httpHeadersCnt;
        std::tie(_httpStatusLine, _httpHeaders, httpHeadersCnt) = WebSocketUtil::parseHttpHeader(_httpResponseData);


        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP status line in opening handshake : %s", _httpStatusLine);
        NET4CXX_LOG_DEBUG(gGenLog, "received HTTP headers in opening handshake : %s", StringMapToString(_httpHeaders));

        auto sl = StrUtil::split(_httpStatusLine);
        if (sl.size() < 2) {
            failHandshake(StrUtil::format("Bad HTTP request status line '%s'", _httpStatusLine));
            return;
        }

        auto httpVersion = boost::trim_copy(sl[0]);
        if (httpVersion != "HTTP/1.1") {
            failHandshake(StrUtil::format("Unsupported HTTP version ('%s')", httpVersion));
            return;
        }

        int statusCode;
        try {
            statusCode = std::stoi(boost::trim_copy(sl[1]));
        } catch (...) {
            failHandshake(StrUtil::format("Bad HTTP status code ('%s')", boost::trim_copy(sl[1])));
            return;
        }

        if (statusCode != 101) {
            std::string reason;

            if (sl.size() > 2) {
                sl.erase(sl.begin(), std::next(sl.begin(), 2));
                reason = " - " + boost::join(sl, "");
            }
            failHandshake(StrUtil::format("WebSocket connection upgrade failed (%d%s)", statusCode, reason));
            return;
        }

        auto upgradeIter = _httpHeaders.find("upgrade");
        if (upgradeIter == _httpHeaders.end()) {
            failHandshake("HTTP Upgrade header missing");
            return;
        }
        if (boost::to_lower_copy(boost::trim_copy(upgradeIter->second)) != "websocket") {
            failHandshake(StrUtil::format("HTTP Upgrade header different from 'websocket' (case-insensitive) : %s",
                                          upgradeIter->second));
            return;
        }

        auto connectionIter = _httpHeaders.find("connection");
        if (connectionIter == _httpHeaders.end()) {
            failHandshake("HTTP Connection header missing");
            return;
        }

        bool connectionUpgrade = false;
        for (auto &c: StrUtil::split(connectionIter->second, ',')) {
            if (boost::to_lower_copy(boost::trim_copy(c)) == "upgrade") {
                connectionUpgrade = true;
                break;
            }
        }
        if (!connectionUpgrade) {
            failHandshake(StrUtil::format("HTTP Connection headers do not include "
                                          "'upgrade' value (case-insensitive) : %s", connectionIter->second));
            return;
        }

        auto webSocketAcceptIter = _httpHeaders.find("sec-websocket-accept");
        if (webSocketAcceptIter == _httpHeaders.end()) {
            failHandshake("HTTP Sec-WebSocket-Accept header missing in opening handshake reply");
            return;
        } else {
            if (httpHeadersCnt["sec-websocket-accept"] > 1) {
                failHandshake("HTTP Sec-WebSocket-Accept header appears more than once in opening handshake reply");
                return;
            }

            auto secWebSocketAcceptGot = boost::trim_copy(webSocketAcceptIter->second);

            SHA1Object sha1;
            sha1.update(_webSocketKey + WS_MAGIC);
            auto secWebSocketAccept = Base64::b64encode(sha1.digest());

            if (secWebSocketAcceptGot != secWebSocketAccept) {
                failHandshake(StrUtil::format("HTTP Sec-WebSocket-Accept bogus value : expected %s / got %s",
                                              secWebSocketAccept, secWebSocketAcceptGot));
                return;
            }
        }

        auto webSocketExtensionsIter = _httpHeaders.find("sec-websocket-extensions");
        if (webSocketExtensionsIter != _httpHeaders.end()) {
            WebSocketExtensionList webSocketExtensions;
            if (httpHeadersCnt["sec-websocket-extensions"] > 1) {
                failHandshake("HTTP Sec-WebSocket-Extensions header appears more than once "
                              "in opening handshake request");
                return;
            } else {
                webSocketExtensions = parseExtensionsHeader(webSocketExtensionsIter->second);
            }

            for (auto &webSocketExtension: webSocketExtensions) {
                auto &extension = webSocketExtension.first;
                auto &params = webSocketExtension.second;
                NET4CXX_LOG_DEBUG(gGenLog, "parsed WebSocket extension '%s'", extension);
                auto PMCE = PERMESSAGE_COMPRESSION_EXTENSION.find(extension);
                if (PMCE != PERMESSAGE_COMPRESSION_EXTENSION.end()) {

                    if (_perMessageCompress) {
                        failHandshake("multiple occurrence of a permessage-compress extension");
                        return;
                    }
                    PerMessageCompressResponsePtr pmceResponse;
                    try {
                        pmceResponse = PMCE->second->createResponseFromParams(params);
                    } catch (std::exception &e) {
                        failHandshake(e.what());
                        return;
                    }

                    PerMessageCompressResponseAcceptPtr accept;
                    if (_perMessageCompressionAccept) {
                        accept = _perMessageCompressionAccept(std::move(pmceResponse));
                    }

                    if (!accept) {
                        failHandshake("WebSocket permessage-compress extension response from server denied by client");
                        return;
                    }

                    _perMessageCompress = PMCE->second->createFromResponseAccept(false, std::move(accept));
                    _webSocketExtensionsInUse.emplace_back(_perMessageCompress);

                } else {
                    failHandshake(StrUtil::format("server wants to use extension '%s' we did not request, "
                                                  "haven't implemented or did not enable", extension));
                    return;
                }
            }
        }

        auto webSocketProtocolIter = _httpHeaders.find("sec-websocket-protocol");
        if (webSocketProtocolIter != _httpHeaders.end()) {
            if (httpHeadersCnt["sec-websocket-protocol"] > 1) {
                failHandshake("HTTP Sec-WebSocket-Protocol header appears more than once in opening handshake reply");
                return;
            }
            auto sp = boost::trim_copy(webSocketProtocolIter->second);
            if (!sp.empty()) {
                if (std::find(factory->getProtocols().begin(), factory->getProtocols().end(), sp) ==
                    factory->getProtocols().end()) {
                    failHandshake(StrUtil::format("subprotocol selected by server (%s) not in subprotocol "
                                                  "list requested by client", sp));
                    return;
                } else {
                    _webSocketProtocolInUse = std::move(sp);
                }
            }
        }

        _data.erase(_data.begin(), std::next(_data.begin(), ((const Byte *)endOfHeader + 4 - _data.data())));
        _state = State::OPEN;

        if (!_openHandshakeTimeoutCall.cancelled()) {
            NET4CXX_LOG_DEBUG(gGenLog, "cancel openHandshakeTimeoutCall");
            _openHandshakeTimeoutCall.cancel();
        }

        _insideMessage = false;
        _currentFrame = boost::none;
        _webSocketVersion = _version;

        if (_autoPingInterval > 0.0) {
            _autoPingPendingCall = reactor()->callLater(_autoPingInterval, [self = shared_from_this(), this](){
                sendAutoPing();
            });
        }

        bool onConnectSuccess = false;
        try {
            ConnectionResponse response(_peer, _httpHeaders, _webSocketVersion, _webSocketProtocolInUse,
                                        _webSocketExtensionsInUse);
            onConnect(std::move(response));
            onConnectSuccess = true;
        } catch (std::exception &e) {
            failConnection(1000, e.what());
        }
        if (onConnectSuccess) {
            if (_trackedTimings) {
                _trackedTimings->track("onOpen");
            }
            onOpen();
        }

        if (!_data.empty()) {
            consumeData();
        }
    }
}

void WebSocketClientProtocol::startProxyConnect() {
    auto factory = getFactory<WebSocketClientFactory>();
    std::string request = StrUtil::format("CONNECT %s:%u HTTP/1.1\x0d\x0a", factory->getHost(), factory->getPort());
    request += StrUtil::format("Host: %s:%u\x0d\x0a", factory->getHost(), factory->getPort());
    request += "\x0d\x0a";

    NET4CXX_LOG_DEBUG(gGenLog, "request: %s", request);
    sendData((const Byte *)request.data(), request.size());
}

void WebSocketClientProtocol::startHandshake() {
    auto factory = getFactory<WebSocketClientFactory>();

    std::string request = StrUtil::format("GET %s HTTP/1.1\x0d\x0a", factory->getResource());

    if (!factory->getUserAgent().empty()) {
        request += StrUtil::format("User-Agent: %s\x0d\x0a", factory->getUserAgent());
    }

    request += StrUtil::format("Host: %s:%u\x0d\x0a", factory->getHost(), factory->getPort());
    request += "Upgrade: WebSocket\x0d\x0a";
    request += "Connection: Upgrade\x0d\x0a";
    request += "Pragma: no-cache\x0d\x0a";
    request += "Cache-Control: no-cache\x0d\x0a";

    for (auto &uh: factory->getHeaders()) {
        request += StrUtil::format("%s: %s\x0d\x0a", uh.first, uh.second.front());
    }

    _webSocketKey.resize(16);
    Random::randBytes((Byte *)_webSocketKey.data(), _webSocketKey.size());
    _webSocketKey = Base64::b64encode(_webSocketKey);
    request += StrUtil::format("Sec-WebSocket-Key: %s\x0d\x0a", _webSocketKey);

    if (!factory->getOrigin().empty()) {
        if (_version > 10) {
            request += StrUtil::format("Origin: %s\x0d\x0a", factory->getOrigin());
        } else {
            request += StrUtil::format("Sec-WebSocket-Origin: %s\x0d\x0a", factory->getOrigin());
        }
    }

    if (!factory->getProtocols().empty()) {
        request += StrUtil::format("Sec-WebSocket-Protocol: %s\x0d\x0a", boost::join(factory->getProtocols(), ","));
    }

    StringVector extensions;

    for (auto &offer: _perMessageCompressionOffers) {
        extensions.emplace_back(offer->getExtensionString());
    }

    if (!extensions.empty()) {
        request += StrUtil::format("Sec-WebSocket-Extensions: %s\x0d\x0a", boost::join(extensions, ", "));
    }

    request += StrUtil::format("Sec-WebSocket-Version: %d\x0d\x0a", SPEC_TO_PROTOCOL_VERSION.at(_version));
    request += "\x0d\x0a";

    _httpRequestData = std::move(request);
    sendData((const Byte *)_httpRequestData.data(), _httpRequestData.size());

    NET4CXX_LOG_DEBUG(gGenLog, "request: %s", _httpRequestData);
}


void WebSocketClientFactory::setSessionParameters(std::string url, std::string origin, StringVector protocols,
                                                  std::string useragent, WebSocketHeaders headers, std::string proxy) {
    if (useragent.empty()) {
        useragent = NET4CXX_VER;
    }
    bool isSecure;
    std::string host, resource, path;
    unsigned short port;
    QueryArgListMap params;
    std::tie(isSecure, host, port, resource, path, params) = WebSocketUtil::parseUrl(
            url.empty() ? "ws://localhost" : url);
    _url = std::move(url);
    _isSecure = isSecure;
    _host = std::move(host);
    _port = port;
    _resource = std::move(resource);
    _path = std::move(path);
    _params = std::move(params);

    _origin = std::move(origin);
    _protocols = std::move(protocols);
    _useragent = std::move(useragent);
    _headers = std::move(headers);

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