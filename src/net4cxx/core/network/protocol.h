//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_PROTOCOL_H
#define NET4CXX_CORE_NETWORK_PROTOCOL_H

#include "net4cxx/common/common.h"
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
    virtual void startedConnecting(Connector *connector) = 0;

    virtual void clientConnectionFailed(Connector *connector, std::exception_ptr reason) = 0;

    virtual void clientConnectionLost(Connector *connector, std::exception_ptr reason) = 0;
};


class NET4CXX_COMMON_API Protocol {
public:
    virtual ~Protocol() = default;

    virtual void connectionMade() = 0;

    virtual void dataReceived(Byte *data, size_t length) = 0;

    virtual void connectionLost(std::exception_ptr reason) = 0;

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

    void write(const Byte *data, size_t length) {
        _transport->write(data, length);
    }

    void loseConnection() {
        _transport->loseConnection();
    }

    void abortConnection() {
        _transport->abortConnection();
    }
protected:
    bool _connected{false};
    Connection *_transport{nullptr};
};

NS_END

#endif //NET4CXX_CORE_NETWORK_PROTOCOL_H
