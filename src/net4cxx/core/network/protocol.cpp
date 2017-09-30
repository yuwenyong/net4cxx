//
// Created by yuwenyong on 17-9-26.
//

#include "net4cxx/core/network/protocol.h"
#include "net4cxx/common/utilities/random.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN


void Factory::doStart() {
    if (!_numPorts) {
        startFactory();
    }
    ++_numPorts;
}

void Factory::doStop() {
    if (!_numPorts) {
        return;
    }
    --_numPorts;
    if (!_numPorts) {
        stopFactory();
    }
}

void Factory::startFactory() {

}

void Factory::stopFactory() {

}


void ClientFactory::startedConnecting(Connector *connector) {

}

void ClientFactory::clientConnectionFailed(Connector *connector, std::exception_ptr reason) {

}

void ClientFactory::clientConnectionLost(Connector *connector, std::exception_ptr reason) {

}


std::shared_ptr<Protocol> OneShotFactory::buildProtocol(const Address &address) {
    return _protocol;
}


void ReconnectingClientFactory::clientConnectionFailed(Connector *connector, std::exception_ptr reason) {
    if (_continueTring) {
        retry(connector);
    } else {
        _connector = nullptr;
    }
}

void ReconnectingClientFactory::clientConnectionLost(Connector *connector, std::exception_ptr reason) {
    if (_continueTring) {
        retry(connector);
    } else {
        _connector = nullptr;
    }
}

void ReconnectingClientFactory::stopTrying() {
    if (!_callId.cancelled()) {
        _callId.cancel();
        _connector = nullptr;
    }
    if (_connector) {
        try {
            Connector *connector = _connector;
            _connector = nullptr;
            connector->stopConnecting();
        } catch (NotConnectingError &e) {

        }
    }
}

void ReconnectingClientFactory::retry(Connector *connector) {
    if (!_continueTring) {
        NET4CXX_INFO(gGenLog, "Abandoning reconnect on explicit request");
        return;
    }
    ++_retries;
    if (_maxRetries > 0 && _retries > _maxRetries) {
        NET4CXX_INFO(gGenLog, "Abandoning reconnect after %d retries", _retries);
        return;
    }
    _delay = std::min(_delay * factor, maxDelay);
    _delay = Random::normalvariate(_delay, _delay * jitter);
    NET4CXX_INFO(gGenLog, "Reconnect will retry in %f seconds", _delay);
    _connector = connector;
    _callId = connector->reactor()->callLater(_delay, [self = connector->shared_from_this()]() {
        self->startConnecting();
    });
}

const double ReconnectingClientFactory::maxDelay = 3600.0;
const double ReconnectingClientFactory::initialDelay = 1.0;
const double ReconnectingClientFactory::factor = 2.7182818284590451;
const double ReconnectingClientFactory::jitter = 0.11962656472;


void Protocol::connectionMade() {

}

void Protocol::connectionLost(std::exception_ptr reason) {

}

NS_END