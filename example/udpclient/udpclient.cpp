//
// Created by yuwenyong on 17-11-23.
//

#include "net4cxx/net4cxx.h"


using namespace net4cxx;


class MyProtocol: public DatagramProtocol, public std::enable_shared_from_this<MyProtocol> {
public:
    void startProtocol() override {
        write("Hello boy!");
    }

    void datagramReceived(Byte *datagram, size_t length, Address address) override {
        std::string s((char *)datagram, (char *)datagram + length);
        NET4CXX_LOG_INFO(gAppLog, "Datagram received: %s From %s:%u", s.c_str(), address.getAddress().c_str(),
                     address.getPort());
    }
};


int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
//    reactor.connectUDP("127.0.0.1", 28002, std::make_shared<MyProtocol>());
    reactor.connectUNIXDatagram("/data/foo/bar", std::make_shared<MyProtocol>(), 8192, "/data/foo/bar2");
    reactor.run();
    return 0;
}