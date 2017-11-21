//
// Created by yuwenyong on 17-9-29.
//

#include "net4cxx/net4cxx.h"


using namespace net4cxx;


class MyProtocol: public Protocol, public std::enable_shared_from_this<MyProtocol> {
public:
    void connectionMade() override {
        NET4CXX_INFO(gAppLog, "Connection made");

    }

    void dataReceived(Byte *data, size_t length) override {
        std::string s((char *)data, (char *)data + length);
        NET4CXX_INFO(gAppLog, "Data received: %s", s.c_str());
        write(data, length);
    }

    void connectionLost(std::exception_ptr reason) override {
        NET4CXX_INFO(gAppLog, "Connection lost");
    }
};


class MyFactory: public Factory {
public:
    ProtocolPtr buildProtocol(const Address &address) override {
        NET4CXX_INFO(gAppLog, "Build protocol");
        return std::make_shared<MyProtocol>();
    }
};


int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
//    TCPServerEndpoint endpoint(&reactor, "28001");
//    endpoint.listen(std::make_unique<MyFactory>());
//    serverFromString(&reactor, "tcp:28001")->listen(std::make_unique<MyFactory>());
//    serverFromString(&reactor, "ssl:28001:privateKey=test.key:certKey=test.crt")->listen(std::make_unique<MyFactory>());
    serverFromString(&reactor, "unix:/data/foo/bar")->listen(std::make_unique<MyFactory>());
    reactor.run();
    return 0;
}