//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_PROTOCOL_H
#define NET4CXX_CORE_NETWORK_PROTOCOL_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/global/loggers.h"


NS_BEGIN

class Connection;
class Protocol;

class NET4CXX_COMMON_API Factory {
public:
    virtual ~Factory() = default;

    virtual const char* logPrefix() const;

    void doStart();

    void doStop();

    virtual void startFactory();

    virtual void stopFactory();

    virtual std::shared_ptr<Protocol> buildProtocol(const std::string &address, unsigned short port) = 0;
protected:
    int _numPorts{0};
};


class NET4CXX_COMMON_API BaseProtocol {
public:
    virtual ~BaseProtocol() = default;

    void makeConnection(std::shared_ptr<Connection> transport) {
        _connected = true;
        _transport = std::move(transport);
        connectionMade();
    }

    virtual void connectionMade() = 0;
protected:
    bool _connected{false};
    std::shared_ptr<Connection> _transport;
};


class NET4CXX_COMMON_API Protocol: public BaseProtocol {
public:
    virtual const char* logPrefix() const;

    virtual void dataReceived(Byte *data, size_t length) = 0;

    virtual void connectionLost(std::exception_ptr reason) = 0;
};

NS_END

#endif //NET4CXX_CORE_NETWORK_PROTOCOL_H
