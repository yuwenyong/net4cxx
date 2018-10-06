//
// Created by yuwenyong on 17-11-21.
//

#ifndef NET4CXX_CORE_NETWORK_UNIX_H
#define NET4CXX_CORE_NETWORK_UNIX_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/core/network/base.h"
#include "net4cxx/shared/global/constants.h"
#include "net4cxx/shared/global/loggers.h"

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

NS_BEGIN

class Factory;
class ClientFactory;
class UNIXConnector;


class NET4CXX_COMMON_API UNIXConnection: public Connection, public std::enable_shared_from_this<UNIXConnection> {
public:
    using SocketType = boost::asio::local::stream_protocol::socket;

    UNIXConnection(const ProtocolPtr &protocol, Reactor *reactor);

    SocketType& getSocket() {
        return _socket;
    }

    void write(const Byte *data, size_t length) override;

    void loseConnection() override;

    void abortConnection() override;

    bool getNoDelay() const override {
        return true;
    }

    void setNoDelay(bool enabled) override {

    }

    bool getKeepAlive() const override {
        boost::asio::socket_base::keep_alive option;
        _socket.get_option(option);
        return option.value();
    }

    void setKeepAlive(bool enabled) override {
        boost::asio::socket_base::keep_alive option(enabled);
        _socket.set_option(option);
    }

    std::string getLocalAddress() const override {
        auto endpoint = _socket.local_endpoint();
        return endpoint.path();
    }

    unsigned short getLocalPort() const override {
        return 0;
    }

    std::string getRemoteAddress() const override {
        auto endpoint = _socket.remote_endpoint();
        return endpoint.path();
    }

    unsigned short getRemotePort() const override {
        return 0;
    }
protected:
    void doClose();

    void doAbort();

    virtual void closeSocket();

    void startReading() {
        if (!_reading) {
            doRead();
        }
    }

    void doRead();

    void cbRead(const boost::system::error_code &ec, size_t transferredBytes) {
        _reading = false;
        handleRead(ec, transferredBytes);
        if (!_disconnecting && !_disconnected) {
            doRead();
        }
    }

    void handleRead(const boost::system::error_code &ec, size_t transferredBytes);

    void startWriting() {
        if (!_writing) {
            doWrite();
        }
    }

    void doWrite();

    void cbWrite(const boost::system::error_code &ec, size_t transferredBytes) {
        _writing = false;
        handleWrite(ec, transferredBytes);
        if (!_aborting && !_disconnected && !_writeQueue.empty()) {
            doWrite();
        }
    }

    void handleWrite(const boost::system::error_code &ec, size_t transferredBytes);

    SocketType _socket;
    std::exception_ptr _error;
};


class NET4CXX_COMMON_API UNIXServerConnection: public UNIXConnection {
public:
    explicit UNIXServerConnection(Reactor *reactor)
            : UNIXConnection({}, reactor) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::UNIXServerConnectionCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~UNIXServerConnection() override {
        NET4CXX_Watcher->dec(WatchKeys::UNIXServerConnectionCount);
    }
#endif

    void cbAccept(const ProtocolPtr &protocol);
};


class NET4CXX_COMMON_API UNIXClientConnection: public UNIXConnection {
public:
    explicit UNIXClientConnection(Reactor *reactor)
            : UNIXConnection({}, reactor) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::UNIXClientConnectionCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~UNIXClientConnection() override {
        NET4CXX_Watcher->dec(WatchKeys::UNIXClientConnectionCount);
    }
#endif

    void cbConnect(const ProtocolPtr &protocol, std::shared_ptr<UNIXConnector> connector);
protected:
    void closeSocket() override;

    std::shared_ptr<UNIXConnector> _connector;
};


class NET4CXX_COMMON_API UNIXListener: public Listener, public std::enable_shared_from_this<UNIXListener> {
public:
    using AcceptorType = boost::asio::local::stream_protocol::acceptor;
    using SocketType = boost::asio::local::stream_protocol::socket;
    using EndpointType = boost::asio::local::stream_protocol::endpoint;

    UNIXListener(std::string path, std::shared_ptr<Factory> factory, Reactor *reactor);

#ifdef NET4CXX_DEBUG
    ~UNIXListener() override {
        NET4CXX_Watcher->dec(WatchKeys::UNIXListenerCount);
    }
#endif

    void startListening() override;

    void stopListening() override;

    std::string getLocalAddress() const {
        auto endpoint = _acceptor.local_endpoint();
        return endpoint.path();
    }

    unsigned short getLocalPort() const {
        return 0;
    }
protected:
    void cbAccept(const boost::system::error_code &ec);

    void handleAccept(const boost::system::error_code &ec);

    void doAccept() {
        _connection = std::make_shared<UNIXServerConnection>(_reactor);
        _acceptor.async_accept(_connection->getSocket(), std::bind(&UNIXListener::cbAccept, shared_from_this(),
                                                                   std::placeholders::_1));
    }

    std::string _path;
    std::shared_ptr<Factory> _factory;
    AcceptorType _acceptor;
    bool _connected{false};
    std::shared_ptr<UNIXServerConnection> _connection;
};


class NET4CXX_COMMON_API UNIXConnector: public Connector, public std::enable_shared_from_this<UNIXConnector> {
public:
    using SocketType = boost::asio::local::stream_protocol::socket;
    using EndpointType = boost::asio::local::stream_protocol::endpoint;

    UNIXConnector(std::string path, std::shared_ptr<ClientFactory> factory, double timeout, Reactor *reactor);

#ifdef NET4CXX_DEBUG
    ~UNIXConnector() override {
        NET4CXX_Watcher->dec(WatchKeys::UNIXConnectorCount);
    }
#endif

    void startConnecting() override;

    void stopConnecting() override;

    void connectionFailed(std::exception_ptr reason={});

    void connectionLost(std::exception_ptr reason={});
protected:
    ProtocolPtr buildProtocol(const Address &address);

    void cancelTimeout() {
        if (!_timeoutId.cancelled()) {
            _timeoutId.cancel();
        }
    }

    void doConnect();

    void cbConnect(const boost::system::error_code &ec) {
        handleConnect(ec);
    }

    void handleConnect(const boost::system::error_code &ec);

    void cbTimeout() {
        handleTimeout();
    }

    void handleTimeout();

    void makeTransport();

    enum State {
        kDisconnected,
        kConnecting,
        kConnected,
    };

    std::string _path;
    std::shared_ptr<ClientFactory> _factory;
    double _timeout{0.0};
    std::shared_ptr<UNIXClientConnection> _connection;
    State _state{kDisconnected};
    DelayedCall _timeoutId;
    bool _factoryStarted{false};
    std::exception_ptr _error;
};


class NET4CXX_COMMON_API UNIXDatagramConnection: public DatagramConnection,
                                                 public std::enable_shared_from_this<UNIXDatagramConnection> {
public:
    using SocketType = boost::asio::local::datagram_protocol::socket;
    using EndpointType = boost::asio::local::datagram_protocol::endpoint;

    UNIXDatagramConnection(std::string path, const DatagramProtocolPtr &protocol, size_t maxPacketSize,
                           Reactor *reactor);

    UNIXDatagramConnection(std::string path, const DatagramProtocolPtr &protocol, size_t maxPacketSize,
                           std::string bindPath, Reactor *reactor);

#ifdef NET4CXX_DEBUG
    ~UNIXDatagramConnection() override {
        NET4CXX_Watcher->dec(WatchKeys::UNIXDatagramConnectionCount);
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
};

NS_END

#endif //BOOST_ASIO_HAS_LOCAL_SOCKETS

#endif //NET4CXX_CORE_NETWORK_UNIX_H
