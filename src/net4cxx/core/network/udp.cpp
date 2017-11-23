//
// Created by yuwenyong on 17-11-22.
//

#include "net4cxx/core/network/udp.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN


UDPConnection::UDPConnection(unsigned short port, const DatagramProtocolPtr &protocol, std::string interface,
                             size_t maxPacketSize, Reactor *reactor)
        : DatagramConnection({(interface.empty() ? "0.0.0.0" : std::move(interface)), port}, protocol, maxPacketSize,
                             reactor)
        , _socket(reactor->getService()) {
#ifndef NET4CXX_NDEBUG
    NET4CXX_Watcher->inc(NET4CXX_UDPConnection_COUNT);
#endif
}

UDPConnection::UDPConnection(std::string address, unsigned short port, const DatagramProtocolPtr &protocol,
                             size_t maxPacketSize, Address bindAddress, Reactor *reactor)
        : DatagramConnection({std::move(address), port}, protocol, maxPacketSize, std::move(bindAddress), reactor)
        , _socket(reactor->getService()) {
#ifndef NET4CXX_NDEBUG
    NET4CXX_Watcher->inc(NET4CXX_UDPConnection_COUNT);
#endif
}

void UDPConnection::write(const Byte *datagram, size_t length, const Address &address) {
    try {
        if (_connectedAddress) {
            BOOST_ASSERT(!address || address == _connectedAddress);
            _socket.send(boost::asio::buffer(datagram, length));
        } else {
            BOOST_ASSERT(address);
            EndpointType receiver(AddressType::from_string(address.getAddress()), address.getPort());
            if (!_socket.is_open()) {
                _socket.open(receiver.protocol());
            }
            _socket.send_to(boost::asio::buffer(datagram, length), receiver);
        }
    } catch (boost::system::system_error &e) {
        NET4CXX_INFO(gGenLog, "Write error %d: %s", e.code().value(), e.code().message().c_str());
        if (_connectedAddress) {
            connectionRefused();
        }
        throw;
    }
}

void UDPConnection::connect(const Address &address) {
    if (_connectedAddress) {
        NET4CXX_THROW_EXCEPTION(AlreadyConnected, "Reconnecting is not currently supported");
    }
    _connectedAddress = address;
    connectSocket();
}

void UDPConnection::loseConnection() {
    if (_reading && _socket.is_open()) {
        _socket.close();
    }
}

std::string UDPConnection::getLocalAddress() const {
    auto endpoint = _socket.remote_endpoint();
    return endpoint.address().to_string();
}

unsigned short UDPConnection::getLocalPort() const {
    auto endpoint = _socket.remote_endpoint();
    return endpoint.port();
}

std::string UDPConnection::getRemoteAddress() const {
    return _connectedAddress.getAddress();
}

unsigned short UDPConnection::getRemotePort() const {
    return _connectedAddress.getPort();
}

void UDPConnection::startListening() {
    try {
        if (_bindAddress) {
            bindSocket();
        }
        if (_connectedAddress) {
            connectSocket();
        }
        connectToProtocol();
    } catch (...) {
        connectionFailed(std::current_exception());
    }
}

void UDPConnection::connectToProtocol() {
    auto protocol = _protocol.lock();
    BOOST_ASSERT(protocol);
    protocol->makeConnection(shared_from_this());
    startReading();
}

void UDPConnection::doRead() {
    auto protocol = _protocol.lock();
    BOOST_ASSERT(protocol);
    _reading = true;
    _socket.async_receive_from(boost::asio::buffer(_readBuffer.data(), _readBuffer.size()), _sender,
                            [protocol, self = shared_from_this()](const boost::system::error_code &ec,
                                                                  size_t transferredBytes) {
                                self->cbRead(ec, transferredBytes);
                            });
}

void UDPConnection::handleRead(const boost::system::error_code &ec, size_t transferredBytes) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NET4CXX_ERROR(gGenLog, "Read error %d :%s", ec.value(), ec.message().c_str());
            if (_connectedAddress) {
                connectionRefused();
            }
        } else {
            connectionLost();
            _reading = false;
        }
    } else {
        Address sender(_sender.address().to_string(), _sender.port());
        datagramReceived(_readBuffer.data(), transferredBytes, std::move(sender));
    }
}

void UDPConnection::bindSocket() {
    try {
        EndpointType endpoint{AddressType::from_string(_bindAddress.getAddress()), _bindAddress.getPort()};
        _socket.open(endpoint.protocol());
        _socket.bind(endpoint);
        NET4CXX_INFO(gGenLog, "UDPConnection starting on %s: %u", _bindAddress.getAddress().c_str(),
                     _bindAddress.getPort());
    } catch (boost::system::system_error &e) {
        NET4CXX_ERROR(gGenLog, "Bind error %d: %s", e.code().value(), e.code().message().c_str());
        throw;
    }
}

void UDPConnection::connectSocket() {
    try {
        EndpointType endpoint{AddressType::from_string(_connectedAddress.getAddress()), _connectedAddress.getPort()};
        _socket.connect(endpoint);
    } catch (boost::system::system_error &e) {
        NET4CXX_ERROR(gGenLog, "Connect error %d: %s", e.code().value(), e.code().message().c_str());
        throw;
    }
}

NS_END