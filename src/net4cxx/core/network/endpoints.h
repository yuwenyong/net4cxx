//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_ENDPOINTS_H
#define NET4CXX_CORE_NETWORK_ENDPOINTS_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/base.h"

NS_BEGIN


class Reactor;
class Factory;
class Listener;


class NET4CXX_COMMON_API ServerEndpoint {
public:
    virtual ~ServerEndpoint() = default;
    virtual std::shared_ptr<Listener> listen(std::unique_ptr<Factory> &&protocolFactory) = 0;
};


class NET4CXX_COMMON_API TCPServerEndpoint: public ServerEndpoint {
public:
    TCPServerEndpoint(Reactor *reactor, std::string port, std::string interface)
            : _reactor(reactor)
            , _port(std::move(port))
            , _interface(std::move(interface)) {

    }

    std::shared_ptr<Listener> listen(std::unique_ptr<Factory> &&protocolFactory) override;
protected:
    Reactor *_reactor{nullptr};
    std::string _port;
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
