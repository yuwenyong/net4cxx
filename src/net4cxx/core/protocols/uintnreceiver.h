//
// Created by yuwenyong.vincent on 2019-01-05.
//

#ifndef NET4CXX_CORE_PROTOCOLS_UINTNRECEIVER_H
#define NET4CXX_CORE_PROTOCOLS_UINTNRECEIVER_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/common/serialization/byteconvert.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"
#include "net4cxx/shared/global/constants.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(LengthLimitExceededError, Exception);


template <typename ByteOrderT = ByteOrderNative>
class UInt8Receiver: public Protocol {
public:
    using ByteOrderType = ByteOrderT;

#ifdef NET4CXX_DEBUG
    UInt8Receiver() {
        NET4CXX_Watcher->inc(WatchKeys::UInt8ReceiverCount);
    }

    ~UInt8Receiver() override {
        NET4CXX_Watcher->dec(WatchKeys::UInt8ReceiverCount);
    }
#endif

    void dataReceived(Byte *data, size_t length) final {
        Byte *ptr, *current;
        size_t consumed = 0, total;
        uint8_t len;

        if (!_unprocessed.empty()) {
            _unprocessed.insert(_unprocessed.end(), data, data + length);
            ptr = _unprocessed.data();
            total = _unprocessed.size();
        } else {
            ptr = data;
            total = length;
        }

        try {
            while (consumed + 1 <= total) {
                len = *(ptr + consumed);
                if (consumed + 1 + len <= total) {
                    current = ptr + consumed + 1;
                    consumed += 1 + len;
                    messageReceived(current, len);
                } else {
                    break;
                }
            }
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gGenLog, "Error happened while reading data: %s", e.what());
            loseConnection();
        } catch (...) {
            NET4CXX_LOG_ERROR(gGenLog, "Unknown error happened while reading data");
            loseConnection();
        }

        if (!_unprocessed.empty()) {
            if (consumed < total) {
                _unprocessed.erase(_unprocessed.begin(), std::next(_unprocessed.begin(), consumed));
            } else {
                _unprocessed.clear();
            }

        } else {
            if (consumed < total) {
                _unprocessed.insert(_unprocessed.end(), ptr + consumed, ptr + total);
            }
        }
    }

    virtual void messageReceived(Byte *data, size_t length) = 0;

    void sendMessage(const Byte *data, size_t length) {
        if (length > 0xFF) {
            NET4CXX_THROW_EXCEPTION(LengthLimitExceededError, "Message size (%llu) exceeded", length);
        }
        ByteArray message;
        message.push_back((uint8_t)length);
        message.insert(message.end(), data, data + length);
        write(message);
    }

    void sendMessage(const ByteArray &data) {
        sendMessage(data.data(), data.size());
    }

    void sendMessage(const char *data) {
        sendMessage((const Byte *)data, strlen(data));
    }

    void sendMessage(const std::string &data) {
        sendMessage((const Byte *)data.c_str(), data.size());
    }
protected:
    ByteArray _unprocessed;
};


template <typename ByteOrderT = ByteOrderNative>
class UInt8ExtReceiver: public Protocol {
public:
    using ByteOrderType = ByteOrderT;

#ifdef NET4CXX_DEBUG
    UInt8ExtReceiver() {
        NET4CXX_Watcher->inc(WatchKeys::UInt8ExtReceiverCount);
    }

    ~UInt8ExtReceiver() override {
        NET4CXX_Watcher->dec(WatchKeys::UInt8ExtReceiverCount);
    }
#endif

    void dataReceived(Byte *data, size_t length) final {
        Byte *ptr, *current;
        size_t consumed = 0, total, offset;
        uint32_t len;

        if (!_unprocessed.empty()) {
            _unprocessed.insert(_unprocessed.end(), data, data + length);
            ptr = _unprocessed.data();
            total = _unprocessed.size();
        } else {
            ptr = data;
            total = length;
        }

        try {
            while (consumed + 1 <= total) {
                offset = 0;
                len = *(ptr + consumed);
                if (len == 0xFF) {
                    if (consumed + 1 + 4 <= total) {
                        len = *(uint32_t *)(ptr + consumed + 1);
                        ByteConvert::convertFrom(len, ByteOrderType{});
                        offset = 4;
                    } else {
                        break;
                    }
                }
                if (consumed + 1 + offset + len <= total) {
                    current = ptr + consumed + 1 + offset;
                    consumed += 1 + offset + len;
                    messageReceived(current, len);
                } else {
                    break;
                }
            }
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gGenLog, "Error happened while reading data: %s", e.what());
            loseConnection();
        } catch (...) {
            NET4CXX_LOG_ERROR(gGenLog, "Unknown error happened while reading data");
            loseConnection();
        }

        if (!_unprocessed.empty()) {
            if (consumed < total) {
                _unprocessed.erase(_unprocessed.begin(), std::next(_unprocessed.begin(), consumed));
            } else {
                _unprocessed.clear();
            }

        } else {
            if (consumed < total) {
                _unprocessed.insert(_unprocessed.end(), ptr + consumed, ptr + total);
            }
        }
    }

    virtual void messageReceived(Byte *data, size_t length) = 0;

    void sendMessage(const Byte *data, size_t length) {
        ByteArray message;
        if (length >= 0xFF) {
            message.push_back(0xFF);
            uint32_t len = ByteConvert::convertTo((uint32_t)length, ByteOrderType{});
            message.insert(message.end(), (uint8_t *)&len, (uint8_t *)&len + sizeof(len));
        } else {
            message.push_back((uint8_t)length);
        }

        message.insert(message.end(), data, data + length);
        write(message);
    }

    void sendMessage(const ByteArray &data) {
        sendMessage(data.data(), data.size());
    }

    void sendMessage(const char *data) {
        sendMessage((const Byte *)data, strlen(data));
    }

    void sendMessage(const std::string &data) {
        sendMessage((const Byte *)data.c_str(), data.size());
    }
protected:
    ByteArray _unprocessed;
};


template <typename ByteOrderT = ByteOrderNative>
class UInt16Receiver: public Protocol {
public:
    using ByteOrderType = ByteOrderT;

#ifdef NET4CXX_DEBUG
    UInt16Receiver() {
        NET4CXX_Watcher->inc(WatchKeys::UInt16ReceiverCount);
    }

    ~UInt16Receiver() override {
        NET4CXX_Watcher->dec(WatchKeys::UInt16ReceiverCount);
    }
#endif

    void dataReceived(Byte *data, size_t length) final {
        Byte *ptr, *current;
        size_t consumed = 0, total;
        uint16_t len;

        if (!_unprocessed.empty()) {
            _unprocessed.insert(_unprocessed.end(), data, data + length);
            ptr = _unprocessed.data();
            total = _unprocessed.size();
        } else {
            ptr = data;
            total = length;
        }

        try {
            while (consumed + 2 <= total) {
                len = *(uint16_t *)(ptr + consumed);
                len = ByteConvert::convertFrom(len, ByteOrderType{});
                if (consumed + 2 + len <= total) {
                    current = ptr + consumed + 2;
                    consumed += 2 + len;
                    messageReceived(current, len);
                } else {
                    break;
                }
            }
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gGenLog, "Error happened while reading data: %s", e.what());
            loseConnection();
        } catch (...) {
            NET4CXX_LOG_ERROR(gGenLog, "Unknown error happened while reading data");
            loseConnection();
        }

        if (!_unprocessed.empty()) {
            if (consumed < total) {
                _unprocessed.erase(_unprocessed.begin(), std::next(_unprocessed.begin(), consumed));
            } else {
                _unprocessed.clear();
            }

        } else {
            if (consumed < total) {
                _unprocessed.insert(_unprocessed.end(), ptr + consumed, ptr + total);
            }
        }
    }

    virtual void messageReceived(Byte *data, size_t length) = 0;

    void sendMessage(const Byte *data, size_t length) {
        if (length > 0xFFFF) {
            NET4CXX_THROW_EXCEPTION(LengthLimitExceededError, "Message size (%llu) exceeded", length);
        }
        ByteArray message;
        uint16_t len = ByteConvert::convertTo((uint16_t)length, ByteOrderType{});
        message.insert(message.end(), (uint8_t *)&len, (uint8_t *)&len + sizeof(len));
        message.insert(message.end(), data, data + length);
        write(message);
    }

    void sendMessage(const ByteArray &data) {
        sendMessage(data.data(), data.size());
    }

    void sendMessage(const char *data) {
        sendMessage((const Byte *)data, strlen(data));
    }

    void sendMessage(const std::string &data) {
        sendMessage((const Byte *)data.c_str(), data.size());
    }
protected:
    ByteArray _unprocessed;
};


template <typename ByteOrderT = ByteOrderNative>
class UInt16ExtReceiver: public Protocol {
public:
    using ByteOrderType = ByteOrderT;

#ifdef NET4CXX_DEBUG
    UInt16ExtReceiver() {
        NET4CXX_Watcher->inc(WatchKeys::UInt16ExtReceiverCount);
    }

    ~UInt16ExtReceiver() override {
        NET4CXX_Watcher->dec(WatchKeys::UInt16ExtReceiverCount);
    }
#endif

    void dataReceived(Byte *data, size_t length) final {
        Byte *ptr, *current;
        size_t consumed = 0, total, offset;
        uint32_t len;

        if (!_unprocessed.empty()) {
            _unprocessed.insert(_unprocessed.end(), data, data + length);
            ptr = _unprocessed.data();
            total = _unprocessed.size();
        } else {
            ptr = data;
            total = length;
        }

        try {
            while (consumed + 2 <= total) {
                offset = 0;
                len = *(uint16_t *)(ptr + consumed);
                len = ByteConvert::convertFrom((uint16_t)len, ByteOrderType{});
                if (len == 0xFFFF) {
                    if (consumed + 2 + 4 <= total) {
                        len = *(uint32_t *)(ptr + consumed + 2);
                        ByteConvert::convertFrom(len, ByteOrderType{});
                        offset = 4;
                    } else {
                        break;
                    }
                }
                if (consumed + 2 + offset + len <= total) {
                    current = ptr + consumed + 2 + offset;
                    consumed += 2 + offset + len;
                    messageReceived(current, len);
                } else {
                    break;
                }
            }
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gGenLog, "Error happened while reading data: %s", e.what());
            loseConnection();
        } catch (...) {
            NET4CXX_LOG_ERROR(gGenLog, "Unknown error happened while reading data");
            loseConnection();
        }

        if (!_unprocessed.empty()) {
            if (consumed < total) {
                _unprocessed.erase(_unprocessed.begin(), std::next(_unprocessed.begin(), consumed));
            } else {
                _unprocessed.clear();
            }

        } else {
            if (consumed < total) {
                _unprocessed.insert(_unprocessed.end(), ptr + consumed, ptr + total);
            }
        }
    }

    virtual void messageReceived(Byte *data, size_t length) = 0;

    void sendMessage(const Byte *data, size_t length) {
        ByteArray message;
        if (length >= 0xFFFF) {
            message.push_back(0xFF);
            message.push_back(0xFF);
            uint32_t len = ByteConvert::convertTo((uint32_t)length, ByteOrderType{});
            message.insert(message.end(), (uint8_t *)&len, (uint8_t *)&len + sizeof(len));
        } else {
            uint16_t len = ByteConvert::convertTo((uint16_t)length, ByteOrderType{});
            message.insert(message.end(), (uint8_t *)&len, (uint8_t *)&len + sizeof(len));
        }
        message.insert(message.end(), data, data + length);
        write(message);
    }

    void sendMessage(const ByteArray &data) {
        sendMessage(data.data(), data.size());
    }

    void sendMessage(const char *data) {
        sendMessage((const Byte *)data, strlen(data));
    }

    void sendMessage(const std::string &data) {
        sendMessage((const Byte *)data.c_str(), data.size());
    }
protected:
    ByteArray _unprocessed;
};


template <typename ByteOrderT = ByteOrderNative>
class UInt32Receiver: public Protocol {
public:
    using ByteOrderType = ByteOrderT;

#ifdef NET4CXX_DEBUG
    UInt32Receiver() {
        NET4CXX_Watcher->inc(WatchKeys::UInt32ReceiverCount);
    }

    ~UInt32Receiver() override {
        NET4CXX_Watcher->dec(WatchKeys::UInt32ReceiverCount);
    }
#endif

    void dataReceived(Byte *data, size_t length) final {
        Byte *ptr, *current;
        size_t consumed = 0, total;
        uint32_t len;

        if (!_unprocessed.empty()) {
            _unprocessed.insert(_unprocessed.end(), data, data + length);
            ptr = _unprocessed.data();
            total = _unprocessed.size();
        } else {
            ptr = data;
            total = length;
        }

        try {
            while (consumed + 4 <= total) {
                len = *(uint32_t *)(ptr + consumed);
                len = ByteConvert::convertFrom(len, ByteOrderType{});
                if (consumed + 4 + len <= total) {
                    current = ptr + consumed + 4;
                    consumed += 4 + len;
                    messageReceived(current, len);
                } else {
                    break;
                }
            }
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gGenLog, "Error happened while reading data: %s", e.what());
            loseConnection();
        } catch (...) {
            NET4CXX_LOG_ERROR(gGenLog, "Unknown error happened while reading data");
            loseConnection();
        }

        if (!_unprocessed.empty()) {
            if (consumed < total) {
                _unprocessed.erase(_unprocessed.begin(), std::next(_unprocessed.begin(), consumed));
            } else {
                _unprocessed.clear();
            }

        } else {
            if (consumed < total) {
                _unprocessed.insert(_unprocessed.end(), ptr + consumed, ptr + total);
            }
        }
    }

    virtual void messageReceived(Byte *data, size_t length) = 0;

    void sendMessage(const Byte *data, size_t length) {
        if (length > 0xFFFFFFFF) {
            NET4CXX_THROW_EXCEPTION(LengthLimitExceededError, "Message size (%llu) exceeded", length);
        }
        ByteArray message;
        uint32_t len = ByteConvert::convertTo((uint32_t)length, ByteOrderType{});
        message.insert(message.end(), (uint8_t *)&len, (uint8_t *)&len + sizeof(len));
        message.insert(message.end(), data, data + length);
        write(message);
    }

    void sendMessage(const ByteArray &data) {
        sendMessage(data.data(), data.size());
    }

    void sendMessage(const char *data) {
        sendMessage((const Byte *)data, strlen(data));
    }

    void sendMessage(const std::string &data) {
        sendMessage((const Byte *)data.c_str(), data.size());
    }
protected:
    ByteArray _unprocessed;
};


NS_END

#endif //NET4CXX_CORE_PROTOCOLS_UINTNRECEIVER_H
