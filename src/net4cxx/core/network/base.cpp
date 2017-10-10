//
// Created by yuwenyong on 17-9-20.
//

#include "net4cxx/core/network/base.h"
#include <boost/filesystem.hpp>
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"


NS_BEGIN


SSLOptionPtr SSLOption::create(const SSLParams &sslParams) {
    struct EnableMakeShared: public SSLOption {
        explicit EnableMakeShared(const SSLParams &params): SSLOption(params) {}
    };
    if (sslParams.isServerSide()) {
        if (sslParams.getCertFile().empty()) {
            NET4CXX_THROW_EXCEPTION(KeyError, "missing cert file in sslParams");
        }
    } else {
        if (sslParams.getVerifyMode() != SSLVerifyMode::CERT_NONE && sslParams.getVerifyFile().empty()) {
            NET4CXX_THROW_EXCEPTION(KeyError, "missing verify file in sslParams");
        }
    }
    const std::string &certFile = sslParams.getCertFile();
    if (!certFile.empty() && !boost::filesystem::exists(certFile)) {
        NET4CXX_THROW_EXCEPTION(ValueError, StrUtil::format("cert file \"%s\" does not exist", certFile.c_str()));
    }
    const std::string &keyFile = sslParams.getKeyFile();
    if (!keyFile.empty() && !boost::filesystem::exists(keyFile)) {
        NET4CXX_THROW_EXCEPTION(ValueError, StrUtil::format("key file \"%s\" does not exist", certFile.c_str()));
    }
    const std::string &verifyFile = sslParams.getVerifyFile();
    if (!verifyFile.empty() && !boost::filesystem::exists(verifyFile)) {
        NET4CXX_THROW_EXCEPTION(ValueError, StrUtil::format("verify file \"%s\" does not exist", certFile.c_str()));
    }
    auto sslOption = std::make_shared<EnableMakeShared>(sslParams);
    return sslOption;
}

SSLOption::SSLOption(const SSLParams &sslParams)
        : _serverSide(sslParams.isServerSide())
        , _context(boost::asio::ssl::context::sslv23) {
    boost::system::error_code ec;
    _context.set_options(boost::asio::ssl::context::no_sslv3, ec);
    const std::string &certFile = sslParams.getCertFile();
    if (!certFile.empty()) {
        setCertFile(certFile);
    }
    const std::string &keyFile = sslParams.getKeyFile();
    if (!keyFile.empty()) {
        setKeyFile(keyFile);
    }
    const std::string &password = sslParams.getPassword();
    if (!password.empty()) {
        setPassword(password);
    }
    if (!_serverSide) {
        auto verifyMode = sslParams.getVerifyMode();
        setVerifyMode(verifyMode);
        const std::string &verifyFile = sslParams.getVerifyFile();
        if (!verifyFile.empty()) {
            setVerifyFile(verifyFile);
        } else if(verifyMode != SSLVerifyMode::CERT_NONE) {
            setDefaultVerifyPath();
        }
        const std::string &checkHost = sslParams.getCheckHost();
        if (!checkHost.empty()) {
            setCheckHost(checkHost);
        }
    }
}


Timeout::Timeout(Reactor *reactor)
        : _timer(reactor->getService()) {

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
    BOOST_ASSERT(protocol);
    protocol->dataReceived(data, length);
}

void Connection::connectionLost(std::exception_ptr reason) {
    auto protocol = _protocol.lock();
    BOOST_ASSERT(protocol);
    protocol->connectionLost(std::move(reason));
}


NS_END