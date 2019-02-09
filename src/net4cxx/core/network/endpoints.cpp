//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/endpoints.h"
#include "net4cxx/common/utilities/strutil.h"
#include "net4cxx/core/network/defer.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN


void WrappingProtocol::connectionMade() {
    _wrappedProtocol->setFactory(getFactory<WrappingFactory>()->getWrappedFactory());
    _wrappedProtocol->makeConnection(_transport);
    auto connectedDeferred = std::move(_connectedDeferred);
    connectedDeferred->callback(_wrappedProtocol);
}

void WrappingProtocol::dataReceived(Byte *data, size_t length) {
    _wrappedProtocol->dataReceived(data, length);
}

void WrappingProtocol::connectionLost(std::exception_ptr reason) {
    _wrappedProtocol->connectionLost(reason);
}


WrappingFactory::WrappingFactory(std::shared_ptr<ClientFactory> wrappedFactory)
        : _wrappedFactory(std::move(wrappedFactory))
        , _onConnection(makeDeferred()) {

}

void WrappingFactory::doStart() {
    _wrappedFactory->doStart();
}

void WrappingFactory::doStop() {
    _wrappedFactory->doStop();
}

ProtocolPtr WrappingFactory::buildProtocol(const Address &address) {
    auto onConnection = std::move(_onConnection);
    try {
        auto proto = _wrappedFactory->buildProtocol(address);
        if (!proto) {
            NET4CXX_THROW_EXCEPTION(NoProtocol, "");
        }
        return std::make_shared<WrappingProtocol>(onConnection, std::move(proto));
    } catch (...) {
        onConnection->errback();
    }
    return nullptr;
}

void WrappingFactory::startedConnecting(ConnectorPtr connector) {
    std::weak_ptr<Connector> c(connector);
    NET4CXX_ASSERT(_onConnection);
    _onConnection->setCanceller([c](DeferredPtr deferred) {
        if (auto connector = c.lock()) {
            deferred->errback(std::make_exception_ptr(NET4CXX_MAKE_EXCEPTION(ConnectingCancelledError, "")));
            connector->stopConnecting();
        }
    });
}

void WrappingFactory::clientConnectionFailed(ConnectorPtr connector, std::exception_ptr reason) {
    auto onConnection = std::move(_onConnection);
    if (onConnection && !onConnection->called()) {
        onConnection->errback(reason);
    }
}


DeferredPtr TCPServerEndpoint::listen(std::shared_ptr<Factory> protocolFactory) const {
    return executeDeferred([this](std::shared_ptr<Factory> factory) -> ListenerPtr {
        return _reactor->listenTCP(_port, std::move(factory), _interface);
    }, std::move(protocolFactory));
}


DeferredPtr SSLServerEndpoint::listen(std::shared_ptr<Factory> protocolFactory) const {
    return executeDeferred([this](std::shared_ptr<Factory> factory) -> ListenerPtr {
        return _reactor->listenSSL(_port, std::move(factory), _sslOption, _interface);
    }, std::move(protocolFactory));
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

DeferredPtr UNIXServerEndpoint::listen(std::shared_ptr<Factory> protocolFactory) const {
    return executeDeferred([this](std::shared_ptr<Factory> protocolFactory) -> ListenerPtr {
        return _reactor->listenUNIX(_path, std::move(protocolFactory));
    }, std::move(protocolFactory));
}

#endif

ServerEndpointPtr _parseTCP(Reactor *reactor, const StringVector &args, const StringMap &params) {
    std::string port = args[0];
    std::string interface;
    if (params.find("interface") != params.end()) {
        interface = params.at("interface");
    }
    return std::make_shared<TCPServerEndpoint>(reactor, port, std::move(interface));
}

ServerEndpointPtr _parseSSL(Reactor *reactor, const StringVector &args, const StringMap &params) {
    std::string port = args[0];
    std::string interface, privateKey, certKey;
    if (params.find("interface") != params.end()) {
        interface = params.at("interface");
    }
    privateKey = params.at("privateKey");
    if (params.find("certKey") != params.end()) {
        certKey = params.at("certKey");
    }
    SSLServerOptionBuilder builder;
    builder.setKeyFile(privateKey);
    if (!certKey.empty()) {
        builder.setCertFile(certKey);
    }
    return std::make_shared<SSLServerEndpoint>(reactor, port, builder.build(), std::move(interface));
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

ServerEndpointPtr _parseUNIX(Reactor *reactor, const StringVector &args, const StringMap &params) {
    std::string path = args[0];
    return std::make_shared<UNIXServerEndpoint>(reactor, std::move(path));
}

#endif

void _parse(const std::string &description, StringVector &args, StringMap &params) {
    StringVector parsedArgs;
    StringMap parsedParams;
    StringVector sofar;
    std::string current;
    std::string ops{":="};
    for (size_t pos = 0; pos < description.size(); ++pos) {
        if (ops.find(description[pos]) != std::string::npos) {
            sofar.push_back(boost::trim_copy(current));
            if (description[pos] == ':') {
                if (sofar.size() == 1) {
                    parsedArgs.push_back(std::move(sofar[0]));
                } else {
                    parsedParams[std::move(sofar[0])] = std::move(sofar[1]);
                }
                sofar.clear();
                ops = ":=";
            } else {
                ops = ":";
            }
            current.clear();
        } else if (description[pos] == '\\') {
            ++pos;
            current.push_back(description[pos]);
        } else {
            current.push_back(description[pos]);
        }
    }
    if (!current.empty()) {
        sofar.push_back(boost::trim_copy(current));
    }
    if (!sofar.empty()) {
        if (sofar.size() == 1) {
            parsedArgs.push_back(std::move(sofar[0]));
        } else {
            parsedParams[std::move(sofar[0])] = std::move(sofar[1]);
        }
    }
    parsedArgs.swap(args);
    parsedParams.swap(params);
}

std::string _parseServer(const std::string &description, StringVector &args, StringMap &params) {
    _parse(description, args, params);
    std::string endpointType;
    if (!args.empty()) {
        endpointType = std::move(args[0]);
        boost::to_lower(endpointType);
        args.erase(args.begin());
    }
    return endpointType;
}

ServerEndpointPtr serverFromString(Reactor *reactor, const std::string &description) {
    StringVector args;
    StringMap params;
    std::string endpointType = _parseServer(description, args, params);
    if (endpointType == "tcp") {
        return _parseTCP(reactor, args, params);
    } else if (endpointType == "ssl") {
        return _parseSSL(reactor, args, params);
    } else if (endpointType == "unix") {
#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
        return _parseUNIX(reactor, args, params);
#else
        NET4CXX_THROW_EXCEPTION(ValueError, StrUtil::format("Unsupported endpoint type: '%s'", endpointType.c_str()));
#endif
    } else {
        NET4CXX_THROW_EXCEPTION(ValueError, "Unknown endpoint type: '%s'", endpointType);
    }
}


DeferredPtr TCPClientEndpoint::connect(std::shared_ptr<ClientFactory> protocolFactory) const {
    try {
        auto wf = std::make_shared<WrappingFactory>(std::move(protocolFactory));
        auto onConnection = wf->getOnConnection();
        _reactor->connectTCP(_host, _port, std::move(wf), _timeout, _bindAddress);
        return onConnection;
    } catch (...) {
        return failDeferred();
    }
}


DeferredPtr SSLClientEndpoint::connect(std::shared_ptr<ClientFactory> protocolFactory) const {
    try {
        auto wf = std::make_shared<WrappingFactory>(std::move(protocolFactory));
        auto onConnection = wf->getOnConnection();
        _reactor->connectSSL(_host, _port, std::move(wf), _sslOption, _timeout, _bindAddress);
        return onConnection;
    } catch (...) {
        return failDeferred();
    }
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

DeferredPtr UNIXClientEndpoint::connect(std::shared_ptr<ClientFactory> protocolFactory) const {
    try {
        auto wf = std::make_shared<WrappingFactory>(std::move(protocolFactory));
        auto onConnection = wf->getOnConnection();
        _reactor->connectUNIX(_path, std::move(wf), _timeout);
        return onConnection;
    } catch (...) {
        return failDeferred();
    }
}

#endif

ClientEndpointPtr _parseClientTCP(Reactor *reactor, const StringVector &args, const StringMap &params) {
    std::string host, port;
    if (args.size() == 2) {
        host = args[0];
        port = args[1];
    } else if (args.size() == 1) {
        if (params.find("host") != params.end()) {
            port = args[0];
        } else {
            host = args[0];
        }
    }
    decltype(params.begin()) iter;
    if ((iter = params.find("port")) != params.end()) {
        port = iter->second;
    }
    double timeout = 30.0;
    if ((iter = params.find("timeout")) != params.end()) {
        timeout = std::stod(iter->second);
    }
    Address bindAddress;
    if ((iter = params.find("bindAddress")) != params.end()) {
        bindAddress.setAddress(iter->second);
    }
    return std::make_shared<TCPClientEndpoint>(reactor, std::move(host), std::move(port), timeout,
                                               std::move(bindAddress));
}


ClientEndpointPtr _parseClientSSL(Reactor *reactor, const StringVector &args, const StringMap &params) {
    std::string host, port;
    if (args.size() == 2) {
        host = args[0];
        port = args[1];
    } else if (args.size() == 1) {
        if (params.find("host") != params.end()) {
            port = args[0];
        } else {
            host = args[0];
        }
    }
    decltype(params.begin()) iter;
    if ((iter = params.find("port")) != params.end()) {
        port = iter->second;
    }
    double timeout = 30.0;
    if ((iter = params.find("timeout")) != params.end()) {
        timeout = std::stod(iter->second);
    }
    Address bindAddress;
    if ((iter = params.find("bindAddress")) != params.end()) {
        bindAddress.setAddress(iter->second);
    }

    SSLClientOptionBuilder builder;
    if ((iter = params.find("hostname")) != params.end()) {
        builder.setCheckHost(iter->second);
    }
    if ((iter = params.find("certKey")) != params.end()) {
        builder.setCertFile(iter->second);
    }
    if ((iter = params.find("privateKey")) != params.end()) {
        builder.setKeyFile(iter->second);
    }
    if ((iter = params.find("caCertsDir")) != params.end()) {
        builder.setVerifyMode(SSLVerifyMode::CERT_REQUIRED);
        builder.setVerifyFile(iter->second);
    }
    return std::make_shared<SSLClientEndpoint>(reactor, std::move(host), std::move(port), builder.build(), timeout,
                                               std::move(bindAddress));
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

ClientEndpointPtr _parseClientUNIX(Reactor *reactor, const StringVector &args, const StringMap &params) {
    std::string path;
    if (!args.empty()) {
        path = args[0];
    } else {
        path = params.at("path");
    }
    decltype(params.begin()) iter;
    double timeout = 30.0;
    if ((iter = params.find("timeout")) != params.end()) {
        timeout = std::stod(iter->second);
    }
    return std::make_shared<UNIXClientEndpoint>(reactor, std::move(path), timeout);
}

#endif

std::string _parseClient(const std::string &description, StringVector &args, StringMap &params) {
    _parse(description, args, params);
    std::string endpointType;
    if (!args.empty()) {
        endpointType = std::move(args[0]);
        boost::to_lower(endpointType);
        args.erase(args.begin());
    }
    return endpointType;
}


ClientEndpointPtr clientFromString(Reactor *reactor, const std::string &description) {
    StringVector args;
    StringMap params;
    std::string endpointType = _parseClient(description, args, params);
    if (endpointType == "tcp") {
        return _parseClientTCP(reactor, args, params);
    } else if (endpointType == "ssl") {
        return _parseClientSSL(reactor, args, params);
    } else if (endpointType == "unix") {
#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
        return _parseClientUNIX(reactor, args, params);
#else
        NET4CXX_THROW_EXCEPTION(ValueError, StrUtil::format("Unsupported endpoint type: '%s'", endpointType.c_str()));
#endif
    } else {
        NET4CXX_THROW_EXCEPTION(ValueError, "Unknown endpoint type: '%s'", endpointType);
    }
}


DeferredPtr connectProtocol(const ClientEndpoint &endpoint, ProtocolPtr protocol) {
    return endpoint.connect(std::make_unique<OneShotFactory>(std::move(protocol)));
}

NS_END