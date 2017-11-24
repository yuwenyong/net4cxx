//
// Created by yuwenyong on 17-9-20.
//

#include "net4cxx/core/network/reactor.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/common/utilities/random.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/ssl.h"
#include "net4cxx/core/network/tcp.h"
#include "net4cxx/core/network/unix.h"
#include "net4cxx/core/network/udp.h"


NS_BEGIN

thread_local Reactor* Reactor::_current = nullptr;

Reactor::Reactor()
        : _ioService()
        , _signalSet(_ioService) {

}

void Reactor::run(bool installSignalHandlers) {
    if (_running) {
        NET4CXX_THROW_EXCEPTION(ReactorAlreadyRunning, "");
    }
    Random::seed();
    if (_ioService.stopped()) {
        _ioService.reset();
    }
    Reactor *oldCurrent = _current;
    _current = this;
    WorkType work(_ioService);
    startRunning(installSignalHandlers);
    _running = false;
    _current = oldCurrent;
}

void Reactor::stop() {
    if (_ioService.stopped()) {
        NET4CXX_THROW_EXCEPTION(ReactorNotRunning, "Can't stop reactor that isn't running.");
    }
    if (!_stopCallbacks.empty()) {
        _stopCallbacks();
        _stopCallbacks.disconnect_all_slots();
    }
    _ioService.stop();
}

ListenerPtr Reactor::listenTCP(const std::string &port, std::unique_ptr<Factory> &&factory,
                               const std::string &interface) {
    auto l = std::make_shared<TCPListener>(port, std::move(factory), interface, this);
    l->startListening();
    return l;
}

ConnectorPtr Reactor::connectTCP(const std::string &host, const std::string &port,
                                 std::unique_ptr<ClientFactory> &&factory, double timeout, const Address &bindAddress) {
    auto c = std::make_shared<TCPConnector>(host, port, std::move(factory), timeout, bindAddress, this);
    c->startConnecting();
    return c;
}

ListenerPtr Reactor::listenSSL(const std::string &port, std::unique_ptr<Factory> &&factory, SSLOptionPtr sslOption,
                               const std::string &interface) {
    auto l = std::make_shared<SSLListener>(port, std::move(factory), std::move(sslOption), interface, this);
    l->startListening();
    return l;
}

ConnectorPtr Reactor::connectSSL(const std::string &host, const std::string &port,
                                 std::unique_ptr<ClientFactory> &&factory, SSLOptionPtr sslOption, double timeout,
                                 const Address &bindAddress) {
    auto c = std::make_shared<SSLConnector>(host, port, std::move(factory), std::move(sslOption), timeout, bindAddress,
                                            this);
    c->startConnecting();
    return c;
}

DatagramConnectionPtr Reactor::listenUDP(unsigned short port, DatagramProtocolPtr protocol,
                                         const std::string &interface, size_t maxPacketSize, bool listenMultiple) {
    auto l = std::make_shared<UDPConnection>(port, protocol, interface, maxPacketSize, listenMultiple, this);
    l->startListening();
    return l;
}

DatagramConnectionPtr Reactor::connectUDP(const std::string &address, unsigned short port, DatagramProtocolPtr protocol,
                                          size_t maxPacketSize, const Address &bindAddress, bool listenMultiple) {
    auto c = std::make_shared<UDPConnection>(address, port, protocol, maxPacketSize, bindAddress, listenMultiple, this);
    c->startListening();
    return c;
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

ListenerPtr Reactor::listenUNIX(const std::string &path, std::unique_ptr<Factory> &&factory) {
    auto l = std::make_shared<UNIXListener>(path, std::move(factory), this);
    l->startListening();
    return l;
}

ConnectorPtr Reactor::connectUNIX(const std::string &path, std::unique_ptr<ClientFactory> &&factory, double timeout) {
    auto c = std::make_shared<UNIXConnector>(path, std::move(factory), timeout, this);
    c->startConnecting();
    return c;
}

DatagramConnectionPtr Reactor::listenUNIXDatagram(const std::string &path, DatagramProtocolPtr protocol,
                                                  size_t maxPacketSize) {
    auto l = std::make_shared<UNIXDatagramConnection>(path, protocol, maxPacketSize, this);
    l->startListening();
    return l;
}

DatagramConnectionPtr Reactor::connectUNIXDatagram(const std::string &path, DatagramProtocolPtr protocol,
                                                   size_t maxPacketSize, const std::string &bindPath) {
    auto c = std::make_shared<UNIXDatagramConnection>(path, protocol, maxPacketSize, bindPath, this);
    c->startListening();
    return c;
}

#endif

void Reactor::startRunning(bool installSignalHandlers) {
    if (installSignalHandlers) {
        _installSignalHandlers = installSignalHandlers;
        handleSignals();
    }
    _running = true;
    while (!_ioService.stopped()) {
        try {
            _ioService.run();
        } catch (std::exception &e) {
            NET4CXX_ERROR(gAppLog, "Unexpected Exception:%s", e.what());
        } catch (...) {
            NET4CXX_ERROR(gAppLog, "Unknown Exception:%s");
        }
    }
}

void Reactor::handleSignals() {
    _signalSet.cancel();
    _signalSet.clear();
    _signalSet.add(SIGINT);
    _signalSet.add(SIGTERM);
#if defined(SIGBREAK)
    _signalSet.add(SIGBREAK);
#endif
#if defined(SIGQUIT)
    _signalSet.add(SIGQUIT);
#endif
    _signalSet.async_wait(std::bind(&Reactor::onSignal, this,  std::placeholders::_1, std::placeholders::_2));
}

void Reactor::onSignal(const boost::system::error_code &ec, int signalNumber) {
    if (!ec) {
        if (signalNumber == SIGINT) {
            sigInt();
        }
        if (signalNumber == SIGTERM) {
            sigTerm();
        }
#if defined(SIGBREAK)
        if (signalNumber == SIGBREAK) {
            sigBreak();
        }
#endif
#if defined(SIGQUIT)
        if (signalNumber == SIGQUIT) {
            sigQuit();
        }
#endif
    }
}

void Reactor::sigInt() {
    NET4CXX_INFO(gGenLog, "Received SIGINT, shutting down.");
    stop();
}

void Reactor::sigTerm() {
    NET4CXX_INFO(gGenLog, "Received SIGINT, shutting down.");
    stop();
}

void Reactor::sigBreak() {
    NET4CXX_INFO(gGenLog, "Received SIGBREAK, shutting down.");
    stop();
}

void Reactor::sigQuit() {
    NET4CXX_INFO(gGenLog, "Received SIGQUIT, shutting down.");
    stop();
}


NS_END