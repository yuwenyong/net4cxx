//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/endpoints.h"
#include <boost/algorithm/string.hpp>
#include "net4cxx/common/utilities/strutil.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN

ListenerPtr TCPServerEndpoint::listen(std::shared_ptr<Factory> protocolFactory) const {
    return _reactor->listenTCP(_port, std::move(protocolFactory), _interface);
}


ListenerPtr SSLServerEndpoint::listen(std::shared_ptr<Factory> protocolFactory) const {
    return _reactor->listenSSL(_port, std::move(protocolFactory), _sslOption, _interface);
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

ListenerPtr UNIXServerEndpoint::listen(std::shared_ptr<Factory> protocolFactory) const {
    return _reactor->listenUNIX(_path, std::move(protocolFactory));
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


ConnectorPtr TCPClientEndpoint::connect(std::shared_ptr<ClientFactory> protocolFactory) const {
    return _reactor->connectTCP(_host, _port, std::move(protocolFactory), _timeout, _bindAddress);
}


ConnectorPtr SSLClientEndpoint::connect(std::shared_ptr<ClientFactory> protocolFactory) const {
    return _reactor->connectSSL(_host, _port, std::move(protocolFactory), _sslOption, _timeout, _bindAddress);
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

ConnectorPtr UNIXClientEndpoint::connect(std::shared_ptr<ClientFactory> protocolFactory) const {
    return _reactor->connectUNIX(_path, std::move(protocolFactory), _timeout);
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


ConnectorPtr connectProtocol(const ClientEndpoint &endpoint, ProtocolPtr protocol) {
    return endpoint.connect(std::make_unique<OneShotFactory>(std::move(protocol)));
}

NS_END