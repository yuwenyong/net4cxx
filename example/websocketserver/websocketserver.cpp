//
// Created by yuwenyong.vincent on 2018/6/30.
//

#include "net4cxx/net4cxx.h"


using namespace net4cxx;

class BroadcastServerFactory: public WebSocketServerFactory,
                              public std::enable_shared_from_this<BroadcastServerFactory> {
public:
    ProtocolPtr buildProtocol(const Address &address) override;

    using WebSocketServerFactory::WebSocketServerFactory;

    void tick(Reactor *reactor) {
        _tickCount += 1;
        broadcast(StrUtil::format("tick %d from server", _tickCount));
        reactor->callLater(1.0, [reactor, this, self=shared_from_this()]() {
           tick(reactor);
        });
    }

    void registerClient(WebSocketServerProtocolPtr client) {
        if (_clients.find(client) == _clients.end()) {
            NET4CXX_LOG_INFO(gAppLog, "register client %s", client->getPeerName());
            _clients.insert(client);
        }
    }

    void unregisterClient(WebSocketServerProtocolPtr client) {
        if (_clients.find(client) != _clients.end()) {
            NET4CXX_LOG_INFO(gAppLog, "unregister client %s", client->getPeerName());
            _clients.erase(client);
        }
    }

    void broadcast(const std::string &msg) {
        NET4CXX_LOG_INFO(gAppLog, "broacasting message '%s' ..", msg);
        for (auto c: _clients) {
            c->sendMessage(msg);
            NET4CXX_LOG_INFO(gAppLog, "message sent to %s", c->getPeerName());
        }
    }
protected:
    int _tickCount{0};
    std::set<WebSocketServerProtocolPtr> _clients;
};


class BroadcastServerProtocol: public WebSocketServerProtocol {
public:
    void onOpen() override {
        getFactory<BroadcastServerFactory>()->registerClient(getSelf<BroadcastServerProtocol>());
    }

    void onMessage(ByteArray payload, bool isBinary) override {
        if (!isBinary) {
            auto msg = StrUtil::format("%s from %s", BytesToString(payload), getPeerName());
            getFactory<BroadcastServerFactory>()->broadcast(msg);
        }
    }

    void connectionLost(std::exception_ptr reason) override {
        WebSocketServerProtocol::connectionLost(reason);
        getFactory<BroadcastServerFactory>()->unregisterClient(getSelf<BroadcastServerProtocol>());
    }
};

ProtocolPtr BroadcastServerFactory::buildProtocol(const Address &address) {
    return std::make_shared<BroadcastServerProtocol>();
}

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    auto factory = std::make_shared<BroadcastServerFactory>("ws://127.0.0.1:9000");
    listenWS(&reactor, factory);
    factory->tick(&reactor);
    reactor.run();
    return 0;
}