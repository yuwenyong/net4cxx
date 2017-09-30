//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_PROTOCOL_H
#define NET4CXX_CORE_NETWORK_PROTOCOL_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/base.h"


NS_BEGIN


class Protocol;


class NET4CXX_COMMON_API Factory {
public:
    virtual ~Factory() = default;

    void doStart();

    void doStop();

    virtual void startFactory();

    virtual void stopFactory();

    virtual std::shared_ptr<Protocol> buildProtocol(const Address &address) = 0;
protected:
    int _numPorts{0};
};


class NET4CXX_COMMON_API ClientFactory: public Factory {
public:
    virtual void startedConnecting(Connector *connector);

    virtual void clientConnectionFailed(Connector *connector, std::exception_ptr reason);

    virtual void clientConnectionLost(Connector *connector, std::exception_ptr reason);
};


class NET4CXX_COMMON_API OneShotFactory: public ClientFactory {
public:
    explicit OneShotFactory(std::shared_ptr<Protocol> protocol)
            : _protocol(std::move(protocol)) {

    }

    std::shared_ptr<Protocol> buildProtocol(const Address &address) override;
protected:
    std::shared_ptr<Protocol> _protocol;
};


class NET4CXX_COMMON_API ReconnectingClientFactory: public ClientFactory {
public:
    void clientConnectionFailed(Connector *connector, std::exception_ptr reason) override;

    void clientConnectionLost(Connector *connector, std::exception_ptr reason) override;

    void stopTrying();
protected:
    void retry(Connector *connector);

    void resetDelay() {
        BOOST_ASSERT(_callId.cancelled());
        _delay = initialDelay;
        _retries = 0;
        _continueTring = true;
        _connector = nullptr;
    }

    static const double maxDelay;
    static const double initialDelay;
    static const double factor;
    static const double jitter;

    double _delay{initialDelay};
    int _retries{0};
    int _maxRetries{0};
    bool _continueTring{true};
    DelayedCall _callId;
    Connector *_connector{nullptr};
};


class NET4CXX_COMMON_API Protocol {
public:
    friend class Connection;

    virtual ~Protocol() = default;

    virtual void connectionMade();

    virtual void dataReceived(Byte *data, size_t length) = 0;

    virtual void connectionLost(std::exception_ptr reason);

    void makeConnection(Connection *transport) {
        _connected = true;
        _transport = transport;
        connectionMade();
    }

    Reactor* reactor() {
        return _transport ? _transport->reactor() : nullptr;
    }

    Connection* connection() {
        return _transport;
    }

    bool connected() const {
        return _connected;
    }

    void write(const Byte *data, size_t length) {
        BOOST_ASSERT(_transport);
        _transport->write(data, length);
    }

    void write(const ByteArray &data) {
        write(data.data(), data.size());
    }

    void write(const char *data) {
        write((const Byte *)data, strlen(data));
    }

    void write(const std::string &data) {
        write((const Byte *)data.c_str(), data.size());
    }

    void loseConnection() {
        BOOST_ASSERT(_transport);
        _transport->loseConnection();
    }

    void abortConnection() {
        BOOST_ASSERT(_transport);
        _transport->abortConnection();
    }
protected:
    void cbConnectionLost(std::exception_ptr reason) {
        BOOST_ASSERT(_transport);
        _transport = nullptr;
        _connected = false;
        connectionLost(std::move(reason));
    }

    bool _connected{false};
    Connection *_transport{nullptr};
};

NS_END

#endif //NET4CXX_CORE_NETWORK_PROTOCOL_H
