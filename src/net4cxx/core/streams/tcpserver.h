//
// Created by yuwenyong.vincent on 2018/7/14.
//

#ifndef NET4CXX_CORE_STREAMS_TCPSERVER_H
#define NET4CXX_CORE_STREAMS_TCPSERVER_H

#include "net4cxx/common/common.h"
#include "net4cxx/core/streams/iostream.h"


NS_BEGIN

class NET4CXX_COMMON_API TCPServer: public std::enable_shared_from_this<TCPServer> {
public:
    typedef boost::asio::ip::tcp::acceptor AcceptorType;

    TCPServer(Reactor *reactor = nullptr, SSLOptionPtr sslOption = nullptr, size_t maxBufferSize=0);

    virtual ~TCPServer() = default;

    TCPServer(const TCPServer &) = delete;

    TCPServer &operator=(const TCPServer &) = delete;

    void listen(unsigned short port, std::string address = "::");

    void bind(unsigned short port, std::string address);

    void start() {
        doAccept();
    }

    unsigned short getLocalPort() const {
        auto endpoint = _acceptor.local_endpoint();
        return endpoint.port();
    }

    void stop();

    virtual void handleStream(BaseIOStreamPtr stream, std::string address) = 0;

protected:
    void doAccept() {
        _acceptor.async_accept(_socket, std::bind(&TCPServer::onAccept, shared_from_this(), std::placeholders::_1));
    }

    void onAccept(const boost::system::error_code &ec);

    Reactor *_reactor;
    SSLOptionPtr _sslOption;
    AcceptorType _acceptor;
    BaseIOStream::SocketType _socket;
    size_t _maxBufferSize;
};

NS_END

#endif //NET4CXX_CORE_STREAMS_TCPSERVER_H
