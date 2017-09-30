//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/endpoints.h"
#include <boost/algorithm/string.hpp>
#include "net4cxx/common/utilities/strutil.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN

std::shared_ptr<Listener> TCPServerEndpoint::listen(std::unique_ptr<Factory> &&protocolFactory) const {
    return _reactor->listenTCP(_port, std::move(protocolFactory), _interface);
}


std::unique_ptr<ServerEndpoint> _parseTCP(Reactor *reactor, const StringVector &args) {
    std::string port = args[1];
    std::string interface = "::";
    for (size_t i = 2; i != args.size(); ++i) {
        auto fields = StrUtil::partition(args[i], "=");
        if (std::get<1>(fields).empty()) {
            continue;
        }
        if (boost::trim_copy(std::get<0>(fields)) == "interface") {
            interface = boost::trim_copy(std::get<2>(fields));
        }
    }
    return std::make_unique<TCPServerEndpoint>(reactor, port, std::move(interface));
}


std::unique_ptr<ServerEndpoint> serverFromString(Reactor *reactor, const std::string &description) {
    StringVector args = StrUtil::split(boost::to_lower_copy(description), ':');
    if (args.empty()) {
        NET4CXX_THROW_EXCEPTION(ValueError, "Empty endpoint type");
    }
    for (auto &arg: args) {
        boost::trim(arg);
    }
    const std::string &endpointType = args[0];
    if (endpointType == "tcp") {
        return _parseTCP(reactor, args);
    } else {
        NET4CXX_THROW_EXCEPTION(ValueError, StrUtil::format("Unknown endpoint type: '%s'", endpointType.c_str()));
    }
}


std::shared_ptr<Connector> TCPClientEndpoint::connect(std::unique_ptr<ClientFactory> &&protocolFactory) const {
    return _reactor->connectTCP(_host, _port, std::move(protocolFactory), _timeout, _bindAddress);
}


std::shared_ptr<Connector> connectProtocol(const ClientEndpoint &endpoint, std::shared_ptr<Protocol> protocol) {
    return endpoint.connect(std::make_unique<OneShotFactory>(std::move(protocol)));
}

NS_END