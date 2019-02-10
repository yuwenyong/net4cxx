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


class UDPClientApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        reactor()->connectUDP("127.0.0.1", 28002, std::make_shared<MyProtocol>());
//        reactor()->connectUNIXDatagram("/data/bar.sock", std::make_shared<MyProtocol>(), 8192, "/data/bar2.sock");
    }
};


int main(int argc, char **argv) {
    UDPClientApp app;
    app.run(argc, argv);
    return 0;
}