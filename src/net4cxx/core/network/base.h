//
// Created by yuwenyong on 17-9-20.
//

#ifndef NET4CXX_CORE_NETWORK_BASE_H
#define NET4CXX_CORE_NETWORK_BASE_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/algorithm/string.hpp>
#include "net4cxx/common/utilities/errors.h"
#include "net4cxx/common/utilities/messagebuffer.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(AlreadyCancelled, Exception);
NET4CXX_DECLARE_EXCEPTION(ReactorNotRunning, Exception);
NET4CXX_DECLARE_EXCEPTION(ReactorAlreadyRunning, Exception);
NET4CXX_DECLARE_EXCEPTION(AlreadyConnected, Exception);
NET4CXX_DECLARE_EXCEPTION(NotConnectingError, Exception);
NET4CXX_DECLARE_EXCEPTION(ConnectionDone, IOError);
NET4CXX_DECLARE_EXCEPTION(ConnectionAbort, IOError);
NET4CXX_DECLARE_EXCEPTION(UserAbort, Exception);
NET4CXX_DECLARE_EXCEPTION(AlreadyCalledError, Exception);
NET4CXX_DECLARE_EXCEPTION(CancelledError, Exception);


class Reactor;
class Protocol;
using ProtocolPtr = std::shared_ptr<Protocol>;
class SSLOption;
using SSLOptionPtr = std::shared_ptr<SSLOption>;
class DatagramProtocol;
using DatagramProtocolPtr = std::shared_ptr<DatagramProtocol>;


enum class SSLVerifyMode {
    CERT_NONE,
    CERT_OPTIONAL,
    CERT_REQUIRED,
};


class NET4CXX_COMMON_API SSLOption: public boost::noncopyable {
public:
    friend class SSLOptionBuilder;
    friend class SSLClientOptionBuilder;
    friend class SSLServerOptionBuilder;

    typedef boost::asio::ssl::context SSLContextType;

    virtual bool isServerSide() const = 0;

    virtual bool isClientSide() const = 0;

    SSLContextType &context() {
        return _context;
    }
protected:
    SSLOption();

    void setCertFile(const std::string &certFile) {
        _context.use_certificate_chain_file(certFile);
    }

    void setKeyFile(const std::string &keyFile) {
        _context.use_private_key_file(keyFile, boost::asio::ssl::context::pem);
    }

    void setPassword(const std::string &password) {
        _context.set_password_callback([password](size_t, boost::asio::ssl::context::password_purpose) {
            return password;
        });
    }

    void setVerifyMode(SSLVerifyMode verifyMode) {
        if (verifyMode == SSLVerifyMode::CERT_NONE) {
            _context.set_verify_mode(boost::asio::ssl::verify_none);
        } else if (verifyMode == SSLVerifyMode::CERT_OPTIONAL) {
            _context.set_verify_mode(boost::asio::ssl::verify_peer);
        } else {
            _context.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
        }
    }

    void setVerifyFile(const std::string &verifyFile) {
        _context.load_verify_file(verifyFile);
    }

    void setDefaultVerifyPath() {
        _context.set_default_verify_paths();
    }

    void setCheckHost(const std::string &hostName) {
        _context.set_verify_callback(boost::asio::ssl::rfc2818_verification(hostName));
    }

    SSLContextType _context;
};


class NET4CXX_COMMON_API SSLOptionBuilder {
public:
    void setCertFile(const std::string &certFile) {
        _certFile = certFile;
    }

    const std::string& getCertFile() const {
        return _certFile;
    }

    void setKeyFile(const std::string &keyFile) {
        _keyFile = keyFile;
    }

    const std::string& getKeyFile() const {
        return _keyFile;
    }

    void setPassword(const std::string &password) {
        _password = password;
    }

    const std::string& getPassword() const {
        return _password;
    }

    void setVerifyMode(SSLVerifyMode verifyMode) {
        _verifyMode = verifyMode;
    }

    SSLVerifyMode getVerifyMode() const {
        return _verifyMode;
    }

    void setVerifyFile(const std::string &verifyFile) {
        _verifyFile = verifyFile;
    }

    const std::string& getVerifyFile() const {
        return _verifyFile;
    }

    void setCheckHost(const std::string &hostName) {
        _checkHost = hostName;
    }

    const std::string& getCheckHost() const {
        return _checkHost;
    }

    SSLOptionPtr build() const {
        verifyParams();
        auto option = buildOption();
        buildContext(option);
        return option;
    }
protected:
    virtual void verifyParams() const;

    virtual SSLOptionPtr buildOption() const = 0;

    virtual void buildContext(SSLOptionPtr option) const;

    SSLVerifyMode _verifyMode{SSLVerifyMode::CERT_NONE};
    std::string _certFile;
    std::string _keyFile;
    std::string _password;
    std::string _verifyFile;
    std::string _checkHost;
};


class NET4CXX_COMMON_API SSLServerOptionBuilder: public SSLOptionBuilder {
protected:
    void verifyParams() const override;

    SSLOptionPtr buildOption() const override;
};


class NET4CXX_COMMON_API SSLClientOptionBuilder: public SSLOptionBuilder {
protected:
    void verifyParams() const override;

    SSLOptionPtr buildOption() const override;

    void buildContext(SSLOptionPtr option) const override;
};


class NET4CXX_COMMON_API NetUtil {
public:
    static bool isValidIPv4(const std::string &ip) {
        boost::system::error_code ec;
        boost::asio::ip::make_address_v4(ip, ec);
        return !ec;
    }

    static bool isValidIPv6(const std::string &ip) {
        boost::system::error_code ec;
        boost::asio::ip::make_address_v6(ip, ec);
        return !ec;
    }

    static bool isValidIP(const std::string &ip) {
        boost::system::error_code ec;
        boost::asio::ip::make_address(ip, ec);
        return !ec;
    }

    static bool isValidPort(const std::string &port) {
        if (port.empty()) {
            return false;
        }
        return boost::all(port, boost::is_digit());
    }
};

class NET4CXX_COMMON_API Address {
public:
    Address(std::string address="", unsigned short port=0)
            : _address{std::move(address)}
            , _port(port) {

    }

    void setAddress(std::string &&address) {
        _address = std::move(address);
    }

    void setAddress(const std::string &address) {
        _address = address;
    }

    const std::string& getAddress() const {
        return _address;
    }

    void setPort(unsigned short port) {
        _port = port;
    }

    unsigned short getPort() const {
        return _port;
    }

    explicit operator bool() const {
        return !_address.empty();
    }

    bool operator!() const {
        return _address.empty();
    }
protected:
    std::string _address;
    unsigned short _port{0};
};


inline bool operator==(const Address &lhs, const Address &rhs) {
//    return lhs.getAddress() == rhs.getAddress() && lhs.getPort() == rhs.getPort();
    if (lhs.getAddress() != rhs.getAddress()) {
        return false;
    }
    if (NetUtil::isValidIP(lhs.getAddress())) {
        return lhs.getPort() == rhs.getPort();
    } else {
        return true;
    }
}

inline bool operator!=(const Address &lhs, const Address &rhs) {
    return !(lhs == rhs);
}


class NET4CXX_COMMON_API Timeout: public std::enable_shared_from_this<Timeout> {
public:
    friend Reactor;
    friend class DelayedCall;
    typedef boost::asio::steady_timer TimerType;

    explicit Timeout(Reactor *reactor);

    Timeout(const Timeout&) = delete;

    Timeout& operator=(const Timeout&) = delete;
protected:
    template <typename CallbackT>
    void start(const Timestamp &deadline, CallbackT &&callback) {
        _timer.expires_at(deadline);
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void start(const Duration &deadline, CallbackT &&callback) {
        _timer.expires_after(deadline);
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void start(double deadline, CallbackT &&callback) {
        _timer.expires_after(std::chrono::milliseconds(int64_t(deadline * 1000)));
        wait(std::forward<CallbackT>(callback));
    }

    template <typename CallbackT>
    void wait(CallbackT &&callback) {
        _timer.async_wait([callback = std::forward<CallbackT>(callback), timeout = shared_from_this()](
                const boost::system::error_code &ec) {
            if (!ec) {
                callback();
            }
        });
    }

    void cancel() {
        _timer.cancel();
    }

    TimerType _timer;
};


class NET4CXX_COMMON_API DelayedCall {
public:
    DelayedCall() = default;

    explicit DelayedCall(std::weak_ptr<Timeout> timeout)
            : _timeout(std::move(timeout)) {

    }

    bool cancelled() const {
        return _timeout.expired();
    }

    bool active() const {
        return !cancelled();
    }

    void cancel();

    void reset() {
        _timeout.reset();
    }
protected:
    std::weak_ptr<Timeout> _timeout;
};


class NET4CXX_COMMON_API Connection {
public:
    Connection(const ProtocolPtr &protocol, Reactor *reactor)
            : _protocol(protocol)
            , _reactor(reactor) {

    }

    virtual ~Connection() = default;

    virtual void write(const Byte *data, size_t length) = 0;

    virtual void loseConnection() = 0;

    virtual void abortConnection() = 0;

    virtual bool getNoDelay() const = 0;

    virtual void setNoDelay(bool enabled) = 0;

    virtual bool getKeepAlive() const = 0;

    virtual void setKeepAlive(bool enabled) = 0;

    virtual std::string getLocalAddress() const = 0;

    virtual unsigned short getLocalPort() const = 0;

    virtual std::string getRemoteAddress() const = 0;

    virtual unsigned short getRemotePort() const = 0;

    Reactor* reactor() {
        return _reactor;
    }
protected:
    void dataReceived(Byte *data, size_t length);

    void connectionLost(std::exception_ptr reason);

    std::weak_ptr<Protocol> _protocol;
    Reactor *_reactor{nullptr};
    MessageBuffer _readBuffer;
    std::deque<MessageBuffer> _writeQueue;
    bool _reading{false};
    bool _writing{false};
    bool _connected{false};
    bool _disconnected{false};
    bool _disconnecting{false};
    bool _aborting{false};
};

using ConnectionPtr = std::shared_ptr<Connection>;

class NET4CXX_COMMON_API Listener {
public:
    explicit Listener(Reactor *reactor)
            : _reactor(reactor) {

    }

    virtual ~Listener() = default;

    virtual void startListening() = 0;

    virtual void stopListening() = 0;

    Reactor* reactor() {
        return _reactor;
    }
protected:
    Reactor *_reactor{nullptr};
};

using ListenerPtr = std::shared_ptr<Listener>;

class NET4CXX_COMMON_API Connector {
public:
    explicit Connector(Reactor *reactor)
            : _reactor(reactor) {

    }

    virtual ~Connector() = default;

    virtual void startConnecting() = 0;

    virtual void stopConnecting() = 0;

    Reactor* reactor() {
        return _reactor;
    }
protected:
    Reactor *_reactor{nullptr};
};

using ConnectorPtr = std::shared_ptr<Connector>;


class NET4CXX_COMMON_API DatagramConnection {
public:
    DatagramConnection(Address bindAddress, const DatagramProtocolPtr &protocol, size_t maxPacketSize, Reactor *reactor)
            : DatagramConnection({}, protocol, maxPacketSize, std::move(bindAddress), reactor) {

    }

    DatagramConnection(Address connectedAddress, const DatagramProtocolPtr &protocol, size_t maxPacketSize,
                       Address bindAddress, Reactor *reactor)
            : _protocol(protocol)
            , _reactor(reactor)
            , _readBuffer(maxPacketSize)
            , _bindAddress(std::move(bindAddress))
            , _connectedAddress(std::move(connectedAddress)) {

    }

    virtual ~DatagramConnection() = default;

    virtual void write(const Byte *datagram, size_t length, const Address &address) = 0;

    virtual void connect(const Address &address) = 0;

    virtual void loseConnection() = 0;

    virtual bool getBroadcastAllowed() const = 0;

    virtual void setBroadcastAllowed(bool enabled) = 0;

    virtual std::string getLocalAddress() const = 0;

    virtual unsigned short getLocalPort() const = 0;

    virtual std::string getRemoteAddress() const = 0;

    virtual unsigned short getRemotePort() const = 0;

    Reactor* reactor() {
        return _reactor;
    }
protected:
    void datagramReceived(Byte *datagram, size_t length, Address address);

    void connectionFailed(std::exception_ptr error);

    void connectionRefused();

    void connectionLost();

    std::weak_ptr<DatagramProtocol> _protocol;
    Reactor *_reactor{nullptr};
    ByteArray _readBuffer;
    bool _reading{false};
    Address _bindAddress;
    Address _connectedAddress;
};

using DatagramConnectionPtr = std::shared_ptr<DatagramConnection>;

NS_END

#endif //NET4CXX_CORE_NETWORK_BASE_H
