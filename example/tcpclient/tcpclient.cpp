//
// Created by yuwenyong on 17-9-29.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class MyProtocol: public Protocol, public std::enable_shared_from_this<MyProtocol> {
public:
    void connectionMade() override {
        NET4CXX_INFO(gAppLog, "Connection made");
        _timeoutId = reactor()->callLater(3.0f, std::bind(&MyProtocol::sendHello, shared_from_this()));
    }

    void dataReceived(Byte *data, size_t length) override {
        std::string s((char *)data, (char *)data + length);
        NET4CXX_INFO(gAppLog, "Data received: %s", s.c_str());
//        loseConnection();
        _timeoutId = reactor()->callLater(3.0f, std::bind(&MyProtocol::sendHello, shared_from_this()));
    }

    void connectionLost(std::exception_ptr reason) override {
        if (!_timeoutId.cancelled()) {
            _timeoutId.cancel();
        }
        NET4CXX_INFO(gAppLog, "Connection lost");
    }

    void sendHello() {
        std::string s("Hello world");
        write((Byte *)s.c_str(), s.size());
    }

protected:
    DelayedCall _timeoutId;
};


//class MyFactory: public ClientFactory {
//public:
//    std::shared_ptr<Protocol> buildProtocol(const Address &address) override {
//        return std::make_shared<MyProtocol>();
//    }
//
//    void startedConnecting(Connector *connector) override {
//        NET4CXX_INFO(gAppLog, "Start connecting");
//    }
//
//    void clientConnectionFailed(Connector *connector, std::exception_ptr reason) override {
//        NET4CXX_INFO(gAppLog, "Client connection failed");
//    }
//
//    void clientConnectionLost(Connector *connector, std::exception_ptr reason) override {
//        NET4CXX_INFO(gAppLog, "Client connection lost");
//    }
//};

class MyFactory: public ReconnectingClientFactory {
public:
    std::shared_ptr<Protocol> buildProtocol(const Address &address) override {
        resetDelay();
        return std::make_shared<MyProtocol>();
    }

    void startedConnecting(Connector *connector) override {
        NET4CXX_INFO(gAppLog, "Start connecting");
        ReconnectingClientFactory::startedConnecting(connector);
    }

    void clientConnectionFailed(Connector *connector, std::exception_ptr reason) override {
        NET4CXX_INFO(gAppLog, "Client connection failed");
        ReconnectingClientFactory::clientConnectionFailed(connector, std::move(reason));
    }

    void clientConnectionLost(Connector *connector, std::exception_ptr reason) override {
        NET4CXX_INFO(gAppLog, "Client connection lost");
        ReconnectingClientFactory::clientConnectionLost(connector, std::move(reason));
    }
};

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.connectTCP("localhost", "28001", std::make_unique<MyFactory>());
//    TCPClientEndpoint endpoint(&reactor, "localhost", "28001");
//    connectProtocol(endpoint, std::make_shared<MyProtocol>());
    reactor.run();
    return 0;
}