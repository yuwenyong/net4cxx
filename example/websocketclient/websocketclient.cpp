//
// Created by yuwenyong.vincent on 2018/7/1.
//

#include "net4cxx/net4cxx.h"


using namespace net4cxx;


class BroadcastClientProtocol: public WebSocketClientProtocol {
public:
    void onOpen() override {
        sendHello();
    }

    void onMessage(ByteArray payload, bool isBinary) override {
        if (!isBinary) {
            NET4CXX_LOG_INFO(gAppLog, "Text message received: %s", TypeCast<std::string>(payload));
        }
    }

    void sendHello() {
        sendMessage("Hello from client!");
        reactor()->callLater(2.0, [this, self=shared_from_this()](){
            sendHello();
        });
    }
};


class BroadcastClientFactory: public WebSocketClientFactory {
public:
    using WebSocketClientFactory::WebSocketClientFactory;

    ProtocolPtr buildProtocol(const Address &address) override {
        return std::make_shared<BroadcastClientProtocol>();
    }
};


class WebSocketClientApp: public AppBootstrapper {
public:
    using AppBootstrapper::AppBootstrapper;

    void onRun() override {
        auto factory = std::make_shared<BroadcastClientFactory>("ws://127.0.0.1:9000");
        connectWS(reactor(), factory);
    }
};

int main(int argc, char **argv) {
    WebSocketClientApp app;
    app.run(argc, argv);
    return 0;
}