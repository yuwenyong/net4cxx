//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_ENDPOINTS_H
#define NET4CXX_CORE_NETWORK_ENDPOINTS_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/base.h"


NS_BEGIN


class Factory;
class ClientFactory;


class NET4CXX_COMMON_API Endpoint {
public:
    explicit Endpoint(Reactor *reactor)
            : _reactor(reactor) {

    }

    virtual ~Endpoint() = default;

    Reactor* reactor() {
        return _reactor;
    }
protected:
    Reactor *_reactor{nullptr};
};


class NET4CXX_COMMON_API ServerEndpoint: public Endpoint {
public:
    using Endpoint::Endpoint;

    virtual ListenerPtr listen(std::shared_ptr<Factory> protocolFactory) const = 0;
};

using ServerEndpointPtr = std::shared_ptr<ServerEndpoint>;

class NET4CXX_COMMON_API TCPServerEndpoint: public ServerEndpoint {
public:
    TCPServerEndpoint(Reactor *reactor, std::string port, std::string interface={})
            : ServerEndpoint(reactor)
            , _port(std::move(port))
            , _interface(std::move(interface)) {

    }

    ListenerPtr listen(std::shared_ptr<Factory> protocolFactory) const override;
protected:
    std::string _port;
    std::string _interface;
};


class NET4CXX_COMMON_API SSLServerEndpoint: public ServerEndpoint {
public:
    SSLServerEndpoint(Reactor *reactor, std::string port, SSLOptionPtr sslOption, std::string interface={})
            : ServerEndpoint(reactor)
            , _port(std::move(port))
            , _interface(std::move(interface))
            , _sslOption(std::move(sslOption)) {

    }

    ListenerPtr listen(std::shared_ptr<Factory> protocolFactory) const override;
protected:
    std::string _port;
    std::string _interface;
    SSLOptionPtr _sslOption;
};


#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

class NET4CXX_COMMON_API UNIXServerEndpoint: public ServerEndpoint {
public:
    UNIXServerEndpoint(Reactor *reactor, std::string path)
            : ServerEndpoint(reactor)
            , _path(std::move(path)) {

    }

    ListenerPtr listen(std::shared_ptr<Factory> protocolFactory) const override;
protected:
    std::string _path;
};

#endif


///
/// \param reactor
/// \param description
///     tcp:80
///     tcp:80:interface=127.0.0.1
///     ssl:443:privateKey=key.pem:certKey=crt.pem
///     unix:/var/run/finger
/// \return
NET4CXX_COMMON_API ServerEndpointPtr serverFromString(Reactor *reactor, const std::string &description);


class NET4CXX_COMMON_API ClientEndpoint: public Endpoint {
public:
    using Endpoint::Endpoint;

    virtual ConnectorPtr connect(std::shared_ptr<ClientFactory> protocolFactory) const = 0;
};

using ClientEndpointPtr = std::shared_ptr<ClientEndpoint>;

class NET4CXX_COMMON_API TCPClientEndpoint: public ClientEndpoint {
public:
    TCPClientEndpoint(Reactor *reactor, std::string host, std::string port, double timeout=30.0, Address bindAddress={})
            : ClientEndpoint(reactor)
            , _host(std::move(host))
            , _port(std::move(port))
            , _timeout(timeout)
            , _bindAddress(std::move(bindAddress)) {

    }

    ConnectorPtr connect(std::shared_ptr<ClientFactory> protocolFactory) const override;
protected:
    std::string _host;
    std::string _port;
    double _timeout;
    Address _bindAddress;
};


class NET4CXX_COMMON_API SSLClientEndpoint: public ClientEndpoint {
public:
    SSLClientEndpoint(Reactor *reactor, std::string host, std::string port, SSLOptionPtr sslOption, double timeout=30.0,
                      Address bindAddress={})
            : ClientEndpoint(reactor)
            , _host(std::move(host))
            , _port(std::move(port))
            , _sslOption(std::move(sslOption))
            , _timeout(timeout)
            , _bindAddress(std::move(bindAddress)) {

    }

    ConnectorPtr connect(std::shared_ptr<ClientFactory> protocolFactory) const override;
protected:
    std::string _host;
    std::string _port;
    SSLOptionPtr _sslOption;
    double _timeout;
    Address _bindAddress;
};

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

class NET4CXX_COMMON_API UNIXClientEndpoint: public ClientEndpoint {
public:
    UNIXClientEndpoint(Reactor *reactor, std::string path, double timeout=30.0)
            : ClientEndpoint(reactor)
            , _path(std::move(path))
            , _timeout(timeout) {

    }

    ConnectorPtr connect(std::shared_ptr<ClientFactory> protocolFactory) const override;
protected:
    std::string _path;
    double _timeout;
};

#endif

///
/// \param reactor
/// \param description
///     tcp:host=www.example.com:port=80
///     tcp:www.example.com:80
///     tcp:host=www.example.com:80
///     tcp:www.example.com:port=80
///     ssl:web.example.com:443:privateKey=foo.pem:certKey=foo.pem
///     ssl:host=web.example.com:port=443:caCertsDir=/etc/ssl/certs
///     tcp:www.example.com:80:bindAddress=192.0.2.100
///     unix:path=/var/foo/bar:timeout=9
///     unix:/var/foo/bar
///     unix:/var/foo/bar:timeout=9
/// \return
NET4CXX_COMMON_API ClientEndpointPtr clientFromString(Reactor *reactor, const std::string &description);


NET4CXX_COMMON_API ConnectorPtr connectProtocol(const ClientEndpoint &endpoint, ProtocolPtr protocol);

NS_END

#endif //NET4CXX_CORE_NETWORK_ENDPOINTS_H
