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


class NET4CXX_COMMON_API Factory {
public:
    virtual ~Factory() = default;

    void doStart();

    void doStop();

    virtual void startFactory();

    virtual void stopFactory();

    virtual ProtocolPtr buildProtocol(const Address &address) = 0;
protected:
    int _numPorts{0};
};


class NET4CXX_COMMON_API ClientFactory: public Factory {
public:
    virtual void startedConnecting(ConnectorPtr connector);

    virtual void clientConnectionFailed(ConnectorPtr connector, std::exception_ptr reason);

    virtual void clientConnectionLost(ConnectorPtr connector, std::exception_ptr reason);
};


class NET4CXX_COMMON_API OneShotFactory: public ClientFactory {
public:
    explicit OneShotFactory(ProtocolPtr protocol)
            : _protocol(std::move(protocol)) {

    }

    ProtocolPtr buildProtocol(const Address &address) override;
protected:
    ProtocolPtr _protocol;
};


class NET4CXX_COMMON_API ReconnectingClientFactory: public ClientFactory {
public:
    void clientConnectionFailed(ConnectorPtr connector, std::exception_ptr reason) override;

    void clientConnectionLost(ConnectorPtr connector, std::exception_ptr reason) override;

    void stopTrying();

    double getMaxDelay() const {
        return _maxDelay;
    }

    void setMaxDelay(double maxDelay) {
        _maxDelay = maxDelay;
    }

    int getMaxRetires() const {
        return _maxRetries;
    }

    void setMaxRetries(int maxRetries) {
        _maxRetries = maxRetries;
    }
protected:
    void retry(ConnectorPtr connector);

    void resetDelay() {
        BOOST_ASSERT(_callId.cancelled());
        _delay = initialDelay;
        _retries = 0;
        _continueTrying = true;
        _connector.reset();
    }

    static const double maxDelay;
    static const double initialDelay;
    static const double factor;
    static const double jitter;

    double _delay{initialDelay};
    double _maxDelay{maxDelay};
    int _retries{0};
    int _maxRetries{0};
    bool _continueTrying{true};
    DelayedCall _callId;
    std::weak_ptr<Connector> _connector;
};


class NET4CXX_COMMON_API Protocol {
public:
    friend class Connection;

    virtual ~Protocol() = default;

    virtual void connectionMade();

    virtual void dataReceived(Byte *data, size_t length) = 0;

    virtual void connectionLost(std::exception_ptr reason);

    void makeConnection(ConnectionPtr transport) {
        _connected = true;
        _transport = std::move(transport);
        connectionMade();
    }

    Reactor* reactor() {
        return _transport ? _transport->reactor() : nullptr;
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

    bool getNoDelay() const {
        BOOST_ASSERT(_transport);
        return _transport->getNoDelay();
    }

    void setNoDelay(bool enabled) {
        BOOST_ASSERT(_transport);
        _transport->setNoDelay(enabled);
    }

    bool getKeepAlive() const {
        BOOST_ASSERT(_transport);
        return _transport->getKeepAlive();
    }

    void setTcpKeepAlive(bool enabled) {
        BOOST_ASSERT(_transport);
        _transport->setKeepAlive(enabled);
    }

    std::string getLocalAddress() const {
        BOOST_ASSERT(_transport);
        return _transport->getLocalAddress();
    }

    unsigned short getLocalPort() const {
        BOOST_ASSERT(_transport);
        return _transport->getLocalPort();
    }

    std::string getRemoteAddress() const {
        BOOST_ASSERT(_transport);
        return _transport->getRemoteAddress();
    }

    unsigned short getRemotePort() const {
        BOOST_ASSERT(_transport);
        return _transport->getRemotePort();
    }
protected:
    bool _connected{false};
    ConnectionPtr _transport;
};


class NET4CXX_COMMON_API DatagramProtocol {
public:
    friend class DatagramConnection;

    virtual ~DatagramProtocol() = default;

    virtual void startProtocol();

    virtual void stopProtocol();

    virtual void datagramReceived(Byte *datagram, size_t length, Address address) = 0;

    virtual void connectionRefused();

    virtual void connectionFailed(std::exception_ptr reason);

    void makeConnection(DatagramConnectionPtr transport) {
        BOOST_ASSERT(!_transport);
        _transport = std::move(transport);
        doStart();
    }

    Reactor* reactor() {
        return _transport ? _transport->reactor() : nullptr;
    }

    void write(const Byte *datagram, size_t length, const Address &address={}) {
        BOOST_ASSERT(_transport);
        _transport->write(datagram, length, address);
    }

    void write(const ByteArray &datagram, const Address &address={}) {
        write(datagram.data(), datagram.size(), address);
    }

    void write(const char *datagram, const Address &address={}) {
        write((const Byte *)datagram, strlen(datagram), address);
    }

    void write(const std::string &datagram, const Address &address={}) {
        write((const Byte *)datagram.data(), datagram.size(), address);
    }

    void connect(const Address &address) {
        BOOST_ASSERT(_transport);
        _transport->connect(address);
    }

    void loseConnection() {
        BOOST_ASSERT(_transport);
        _transport->loseConnection();
    }

    std::string getLocalAddress() const {
        BOOST_ASSERT(_transport);
        return _transport->getLocalAddress();
    }

    unsigned short getLocalPort() const {
        BOOST_ASSERT(_transport);
        return _transport->getLocalPort();
    }

    std::string getRemoteAddress() const {
        BOOST_ASSERT(_transport);
        return _transport->getRemoteAddress();
    }

    unsigned short getRemotePort() const {
        BOOST_ASSERT(_transport);
        return _transport->getRemotePort();
    }
protected:
    void doStart() {
        if (_numPorts == 0) {
            startProtocol();
        }
        ++_numPorts;
    }

    void doStop() {
        BOOST_ASSERT(_numPorts > 0);
        --_numPorts;
        if (_numPorts == 0) {
            stopProtocol();
            _transport.reset();
        }
    }

    int _numPorts{0};
    DatagramConnectionPtr _transport;
};

NS_END

#endif //NET4CXX_CORE_NETWORK_PROTOCOL_H
