//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_ENDPOINTS_H
#define NET4CXX_CORE_NETWORK_ENDPOINTS_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/global/loggers.h"

NS_BEGIN


class Reactor;
class Factory;
class Port;


class NET4CXX_COMMON_API ServerEndpoint {
public:
    virtual ~ServerEndpoint() = 0;
    virtual std::shared_ptr<Port> listen(std::unique_ptr<Factory> &&protocolFactory) = 0;
};


class NET4CXX_COMMON_API TCPServerEndpoint: public ServerEndpoint {
public:
    TCPServerEndpoint(Reactor *reactor, unsigned short port, std::string interface)
            : _reactor(reactor)
            , _port(port)
            , _interface(std::move(interface)) {

    }

    std::shared_ptr<Port> listen(std::unique_ptr<Factory> &&protocolFactory) override;
protected:
    Reactor *_reactor{nullptr};
    unsigned short _port{0};
    std::string _interface;
};


///
/// \param reactor
/// \param description
///     tcp:80
///     tcp:80:interface=127.0.0.1
///     ssl:443:privateKey=key.pem:certKey=crt.pem
///     unix:/var/run/finger
///     unix:/var/run/finger:mode=660
/// \return
NET4CXX_COMMON_API std::unique_ptr<ServerEndpoint> serverFromString(Reactor *reactor, const std::string &description);


NS_END

#endif //NET4CXX_CORE_NETWORK_ENDPOINTS_H
