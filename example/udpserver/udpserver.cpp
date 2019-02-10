//
// Created by yuwenyong on 17-11-23.
//

#include "net4cxx/net4cxx.h"


using namespace net4cxx;


class MyProtocol: public DatagramProtocol, public std::enable_shared_from_this<MyProtocol> {
public:
    void datagramReceived(Byte *datagram, size_t length, Address address) override {
        std::string s((char *)datagram, (char *)datagram + length);
        NET4CXX_LOG_INFO(gAppLog, "Datagram received: %s From %s:%u", s.c_str(), address.getAddress().c_str(),
                     address.getPort());
        write(datagram, length, address);
    }
};


class UDPServerApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        auto conn = reactor()->listenUDP(28002, std::make_shared<MyProtocol>());
        conn->stopListening()->addCallback([](DeferredValue val) {
            NET4CXX_LOG_INFO(gAppLog, "Stop listening");
            return val;
        });
//        reactor()->listenUNIXDatagram("/data/bar.sock", std::make_shared<MyProtocol>());
    }
};


int main(int argc, char **argv) {
    UDPServerApp app;
    app.run(argc, argv);
    return 0;
}
