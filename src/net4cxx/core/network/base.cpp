//
// Created by yuwenyong on 17-9-20.
//

#include "net4cxx/core/network/base.h"
#include <boost/filesystem.hpp>
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN


SSLOption::SSLOption()
        : _context(boost::asio::ssl::context::sslv23) {
    boost::system::error_code ec;
    _context.set_options(boost::asio::ssl::context::no_sslv3, ec);
}


void SSLOptionBuilder::verifyParams() const {
    if (!_certFile.empty() && !boost::filesystem::exists(_certFile)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "cert file \"%s\" does not exist", _certFile);
    }
    if (!_keyFile.empty() && !boost::filesystem::exists(_keyFile)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "key file \"%s\" does not exist", _keyFile);
    }
    if (!_verifyFile.empty() && !boost::filesystem::exists(_verifyFile)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "verify file \"%s\" does not exist", _verifyFile);
    }
}

void SSLOptionBuilder::buildContext(SSLOptionPtr option) const {
    if (!_certFile.empty()) {
        option->setCertFile(_certFile);
    }
    if (!_keyFile.empty()) {
        option->setKeyFile(_keyFile);
    }
    if (!_password.empty()) {
        option->setPassword(_password);
    }
}


void SSLServerOptionBuilder::verifyParams() const {
    if (_certFile.empty()) {
        NET4CXX_THROW_EXCEPTION(KeyError, "missing cert file in sslOption");
    }
    SSLOptionBuilder::verifyParams();
}

SSLOptionPtr SSLServerOptionBuilder::buildOption() const {
    class SSLServerOption: public SSLOption {
    public:
        bool isServerSide() const override {
            return true;
        }

        bool isClientSide() const override {
            return false;
        }
    };

    auto sslOption = std::make_shared<SSLServerOption>();
    return sslOption;
}


void SSLClientOptionBuilder::verifyParams() const {
    if (_verifyMode != SSLVerifyMode::CERT_NONE && _verifyFile.empty()) {
        NET4CXX_THROW_EXCEPTION(KeyError, "missing verify file in sslOption");
    }
    SSLOptionBuilder::verifyParams();
}

SSLOptionPtr SSLClientOptionBuilder::buildOption() const {
    class SSLClientOption: public SSLOption {
    public:
        bool isServerSide() const override {
            return false;
        }

        bool isClientSide() const override {
            return true;
        }
    };

    auto sslOption = std::make_shared<SSLClientOption>();
    return sslOption;
}

void SSLClientOptionBuilder::buildContext(SSLOptionPtr option) const {
    SSLOptionBuilder::buildContext(option);
    option->setVerifyMode(_verifyMode);
    if (!_verifyFile.empty()) {
        option->setVerifyFile(_verifyFile);
    } else if(_verifyMode != SSLVerifyMode::CERT_NONE) {
        option->setDefaultVerifyPath();
    }
    if (!_checkHost.empty()) {
        option->setCheckHost(_checkHost);
    }
}


Timeout::Timeout(Reactor *reactor)
        : _timer(reactor->getIOContext()) {

}


void DelayedCall::cancel() {
    auto timeout = _timeout.lock();
    if (!timeout) {
        NET4CXX_THROW_EXCEPTION(AlreadyCancelled, "");
    }
    timeout->cancel();
}


void Connection::dataReceived(Byte *data, size_t length) {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    protocol->dataReceived(data, length);
}

void Connection::connectionLost(std::exception_ptr reason) {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    protocol->connectionLost(reason);
}


void DatagramConnection::datagramReceived(Byte *datagram, size_t length, Address address) {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    protocol->datagramReceived(datagram, length, std::move(address));
}

void DatagramConnection::connectionFailed(std::exception_ptr error) {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    protocol->connectionFailed(error);
}

void DatagramConnection::connectionRefused() {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    protocol->connectionRefused();
}

void DatagramConnection::connectionLost() {
    auto protocol = _protocol.lock();
    NET4CXX_ASSERT(protocol);
    protocol->doStop();
}

NS_END