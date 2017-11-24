//
// Created by yuwenyong on 17-11-22.
//

#ifndef NET4CXX_CORE_NETWORK_UDP_H
#define NET4CXX_CORE_NETWORK_UDP_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/base.h"

NS_BEGIN


class NET4CXX_COMMON_API UDPConnection: public DatagramConnection, public std::enable_shared_from_this<UDPConnection> {
public:
    using AddressType = boost::asio::ip::address;
    using SocketType = boost::asio::ip::udp::socket;
    using EndpointType = boost::asio::ip::udp::endpoint;

    UDPConnection(unsigned short port, const DatagramProtocolPtr &protocol, std::string interface, size_t maxPacketSize,
                  bool listenMultiple, Reactor *reactor);

    UDPConnection(std::string address, unsigned short port, const DatagramProtocolPtr &protocol, size_t maxPacketSize,
                  Address bindAddress, bool listenMultiple, Reactor *reactor);

#ifndef NET4CXX_NDEBUG
    ~UDPConnection() override {
        NET4CXX_Watcher->dec(NET4CXX_UDPConnection_COUNT);
    }
#endif

    void write(const Byte *datagram, size_t length, const Address &address) override;

    void connect(const Address &address) override;

    void loseConnection() override;

    bool getBroadcastAllowed() const override;

    void setBroadcastAllowed(bool enabled) override;

    std::string getLocalAddress() const override;

    unsigned short getLocalPort() const override;

    std::string getRemoteAddress() const override;

    unsigned short getRemotePort() const override;

    void startListening();
protected:
    void connectToProtocol();

    void startReading() {
        if (!_reading) {
            doRead();
        }
    }

    void doRead();

    void cbRead(const boost::system::error_code &ec, size_t transferredBytes) {
        handleRead(ec, transferredBytes);
        if (_reading) {
            doRead();
        }
    }

    void handleRead(const boost::system::error_code &ec, size_t transferredBytes);

    void bindSocket();

    void connectSocket();

    SocketType _socket;
    EndpointType _sender;
    bool _listenMultiple{false};
};

NS_END


#endif //NET4CXX_CORE_NETWORK_UDP_H
