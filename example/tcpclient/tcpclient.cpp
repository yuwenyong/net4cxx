//
// Created by yuwenyong on 17-9-29.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class MyProtocol: public Protocol, public std::enable_shared_from_this<MyProtocol> {
public:
    void connectionMade() override {
        NET4CXX_LOG_INFO(gAppLog, "Connection made");
        std::cout << "Keep alve:" << std::boolalpha << getKeepAlive() << std::endl;
        setKeepAlive(true);
        std::cout << "Keep alve:" << std::boolalpha << getKeepAlive() << std::endl;
        setKeepAlive(false);
        std::cout << "Keep alve:" << std::boolalpha << getKeepAlive() << std::endl;
        _timeoutId = reactor()->callLater(3.0f, std::bind(&MyProtocol::sendHello, shared_from_this()));
    }

    void dataReceived(Byte *data, size_t length) override {
        std::string s((char *)data, (char *)data + length);
        NET4CXX_LOG_INFO(gAppLog, "Data received: %s", s.c_str());
        loseConnection();
        _timeoutId = reactor()->callLater(3.0f, std::bind(&MyProtocol::sendHello, shared_from_this()));
    }

    void connectionLost(std::exception_ptr reason) override {
        NET4CXX_LOG_INFO(gAppLog, "Connection lost");
    }

    void sendHello() {
        std::string s("Hello world");
        write((Byte *)s.c_str(), s.size());
    }

protected:
    DelayedCall _timeoutId;
};


class MyFactory: public ClientFactory {
public:
    std::shared_ptr<Protocol> buildProtocol(const Address &address) override {
        return std::make_shared<MyProtocol>();
    }

    void startedConnecting(ConnectorPtr connector) override {
        NET4CXX_LOG_INFO(gAppLog, "Start connecting");
    }

    void clientConnectionFailed(ConnectorPtr connector, std::exception_ptr reason) override {
        NET4CXX_LOG_INFO(gAppLog, "Client connection failed");
    }

    void clientConnectionLost(ConnectorPtr connector, std::exception_ptr reason) override {
        NET4CXX_LOG_INFO(gAppLog, "Client connection lost");
    }
};

//class MyFactory: public ReconnectingClientFactory {
//public:
//    ProtocolPtr buildProtocol(const Address &address) override {
//        resetDelay();
//        return std::make_shared<MyProtocol>();
//    }
//
//    void startedConnecting(ConnectorPtr connector) override {
//        NET4CXX_LOG_INFO(gAppLog, "Start connecting");
//        ReconnectingClientFactory::startedConnecting(std::move(connector));
//    }
//
//    void clientConnectionFailed(ConnectorPtr connector, std::exception_ptr reason) override {
//        NET4CXX_LOG_INFO(gAppLog, "Client connection failed");
//        ReconnectingClientFactory::clientConnectionFailed(std::move(connector), std::move(reason));
//    }
//
//    void clientConnectionLost(ConnectorPtr connector, std::exception_ptr reason) override {
//        NET4CXX_LOG_INFO(gAppLog, "Client connection lost");
//        ReconnectingClientFactory::clientConnectionLost(std::move(connector), std::move(reason));
//    }
//};


class TCPClientApp: public AppBootstrapper {
public:
    using AppBootstrapper::AppBootstrapper;

    void onRun() override {
//        reactor()->connectTCP("localhost", "28001", std::make_shared<MyFactory>());
        clientFromString(reactor(), "tcp:host=localhost:port=28001")->connect(std::make_shared<MyFactory>())
        ->addCallbacks([](DeferredValue val) {
            auto proto = val.asValue<ProtocolPtr>();
            NET4CXX_ASSERT(proto);
            NET4CXX_LOG_INFO(gAppLog, "Connected");
            return val;
        }, [](DeferredValue val) {
            NET4CXX_LOG_INFO(gAppLog, "Connect failed");
            try {
                val.throwError();
            } catch (std::exception &e) {
                NET4CXX_LOG_ERROR(gAppLog, "%s", e.what());
            }
            return val;
        });
//        clientFromString(reactor(), "ssl:host=localhost:port=28001")->connect(std::make_shared<MyFactory>());
//        clientFromString(reactor(), "unix:/data/foo/bar")->connect(std::make_shared<MyFactory>());
//        TCPClientEndpoint endpoint(reactor(), "localhost", "28001");
//        connectProtocol(endpoint, std::make_shared<MyProtocol>());
    }
};

int main(int argc, char **argv) {
    TCPClientApp app;
    app.run(argc, argv);
    return 0;
}