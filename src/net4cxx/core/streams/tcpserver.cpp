//
// Created by yuwenyong.vincent on 2018/7/14.
//

#include "net4cxx/core/streams/tcpserver.h"


NS_BEGIN


TCPServer::TCPServer(Reactor *reactor, SSLOptionPtr sslOption, size_t maxBufferSize)
        : _reactor(reactor ? reactor: Reactor::current())
        , _sslOption(std::move(sslOption))
        , _acceptor(_reactor->getIOContext())
        , _socket(_reactor->getIOContext())
        , _maxBufferSize(maxBufferSize){

}

void TCPServer::listen(unsigned short port, std::string address) {
    bind(port, std::move(address));
    start();
}

void TCPServer::bind(unsigned short port, std::string address) {
    BaseIOStream::ResolverType resolver(_reactor->getIOContext());
    BaseIOStream::ResolverType::results_type results = resolver.resolve(address, std::to_string(port));
    BaseIOStream::EndpointType endpoint = (*results.cbegin()).endpoint();
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen();
}

void TCPServer::stop() {
    _acceptor.close();
}

void TCPServer::onAccept(const boost::system::error_code &ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            throw boost::system::system_error(ec);
        }
    } else {
        try {
            std::shared_ptr<BaseIOStream> stream;
            if (_sslOption) {
                stream = SSLIOStream::create(std::move(_socket), _sslOption, _reactor, _maxBufferSize);
            } else {
                stream = IOStream::create(std::move(_socket), _reactor, _maxBufferSize);
            }
            stream->onAccept();
            std::string remoteAddress = stream->getRemoteAddress();
            handleStream(std::move(stream), std::move(remoteAddress));
        } catch (std::exception &e) {
            NET4CXX_LOG_ERROR(gAppLog, "Error in connection callback:%s", e.what());
        }
        doAccept();
    }
}

NS_END