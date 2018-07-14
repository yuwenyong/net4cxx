//
// Created by yuwenyong.vincent on 2018/7/14.
//

#ifndef NET4CXX_CORE_IOSTREAM_IOSTREAM_H
#define NET4CXX_CORE_IOSTREAM_IOSTREAM_H

#include "net4cxx/common/common.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/core/network/reactor.h"
#include "net4cxx/global/shared/constants.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(StreamClosedError, IOError);


constexpr size_t DEFAULT_READ_CHUNK_SIZE = 4096;
constexpr size_t DEFAULT_MAX_BUFFER_SIZE = 104857600;


class BaseIOStream: public std::enable_shared_from_this<BaseIOStream> {
public:
    using SocketType = boost::asio::ip::tcp::socket;
    using EndpointType = boost::asio::ip::tcp::endpoint;
    using ResolverType = boost::asio::ip::tcp::resolver;
    using ResolverResultsType = ResolverType::results_type;

    typedef std::function<void ()> CallbackType;
    typedef std::function<void (ByteArray)> ReadCallbackType;
    typedef std::function<void (ByteArray)> StreamingCallbackType;
    typedef std::function<void ()> WriteCallbackType;
    typedef std::function<void ()> CloseCallbackType;
    typedef std::function<void ()> ConnectCallbackType;

    template <typename... Args>
    class Wrapper {
    public:
        Wrapper(std::shared_ptr<BaseIOStream> stream, std::function<void(Args...)> callback)
                : _stream(std::move(stream))
                , _callback(std::move(callback)) {
        }

        Wrapper(Wrapper &&rhs) noexcept
                : _stream(std::move(rhs._stream))
                , _callback(std::move(rhs._callback)) {
            rhs._callback = nullptr;
        }

        Wrapper(const Wrapper &rhs) = delete;

        Wrapper& operator=(Wrapper &&rhs) noexcept {
            _stream = std::move(rhs._stream);
            _callback = std::move(rhs._callback);
            rhs._callback = nullptr;
            return *this;
        }

        Wrapper& operator=(const Wrapper &rhs)= delete;

        ~Wrapper() {
            if (_callback) {
                _stream->clearCallbacks();
            }
        }

        void operator()(Args... args) {
            std::function<void(Args...)> callback = std::move(_callback);
            _callback = nullptr;
            callback(args...);
        }
    protected:
        std::shared_ptr<BaseIOStream> _stream;
        std::function<void(Args...)> _callback;
    };

    using Wrapper1 = Wrapper<>;
    using Wrapper2 = Wrapper<const boost::system::error_code &>;
    using Wrapper3 = Wrapper<const boost::system::error_code &, size_t>;
    using Wrapper4 = Wrapper<const boost::system::error_code &, const EndpointType &>;

    BaseIOStream(SocketType &&socket,
                 Reactor *reactor,
                 size_t maxBufferSize=0,
                 size_t readChunkSize=DEFAULT_READ_CHUNK_SIZE);

    virtual ~BaseIOStream() = default;

    const Reactor* reactor() const {
        return _reactor;
    }

    Reactor* reactor() {
        return _reactor;
    }

    std::string getRemoteAddress() const {
        auto endpoint = _socket.remote_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getRemotePort() const {
        auto endpoint = _socket.remote_endpoint();
        return endpoint.port();
    }

    void start() {
        _socket.non_blocking(true);
        maybeAddErrorListener();
    }

    virtual void clearCallbacks();

    virtual void realConnect(const std::string &address, unsigned short port) = 0;

    virtual void closeSocket() = 0;

    virtual void writeToSocket() = 0;

    virtual void readFromSocket() = 0;

    void connect(const std::string &address, unsigned short port, ConnectCallbackType callback= nullptr);

    void readUntilRegex(const std::string &regex, ReadCallbackType callback);

    void readUntil(std::string delimiter, ReadCallbackType callback);

    void readBytes(size_t numBytes, ReadCallbackType callback, StreamingCallbackType streamingCallback= nullptr);

    void readUntilClose(ReadCallbackType callback, StreamingCallbackType streamingCallback= nullptr);

    void write(const Byte *data, size_t length, WriteCallbackType callback=nullptr);

    void setCloseCallback(CloseCallbackType callback);

    void close(std::exception_ptr error= nullptr);

    bool reading() const {
        return static_cast<bool>(_readCallback);
    }

    bool writing() const {
        return !_writeQueue.empty();
    }

    bool closed() const {
        return _closing || _closed;
    }

    void setNoDelay(bool value) {
        if (!closed()) {
            boost::asio::ip::tcp::no_delay option(value);
            _socket.set_option(option);
        }
    }

    size_t getMaxBufferSize() const {
        return _maxBufferSize;
    }

    std::exception_ptr getError() const {
        return _error;
    }

    void onConnect(const boost::system::error_code &ec);

    void onRead(const boost::system::error_code &ec, size_t transferredBytes);

    void onWrite(const boost::system::error_code &ec, size_t transferredBytes);

    void onClose(const boost::system::error_code &ec);
protected:
    void maybeRunCloseCallback();

    void runCallback(CallbackType callback);

    void setReadCallback(ReadCallbackType &&callback) {
        NET4CXX_ASSERT_MSG(!_readCallback, "Already reading");
        _readCallback = std::move(callback);
    }

    void tryInlineRead() {
        if (readFromBuffer()) {
            return;
        }
        checkClosed();
        if ((_state & S_READ) == S_NONE) {
            readFromSocket();
        }
    }

    size_t readToBuffer(const boost::system::error_code &ec, size_t transferredBytes);

    bool readFromBuffer();

    void checkClosed() const {
        if (closed()) {
            NET4CXX_THROW_EXCEPTION(StreamClosedError, "Stream is closed");
        }
    }

    void maybeAddErrorListener();

    static constexpr int S_NONE = 0x00;
    static constexpr int S_READ = 0x01;
    static constexpr int S_WRITE = 0x04;

    SocketType _socket;
    Reactor *_reactor;
    size_t _maxBufferSize;
    std::exception_ptr _error;
    MessageBuffer _readBuffer;
    std::deque<MessageBuffer> _writeQueue;
    boost::optional<std::string> _readDelimiter;
    boost::optional<boost::regex> _readRegex;
    boost::optional<size_t> _readBytes;
    bool _readUntilClose{false};
    ReadCallbackType _readCallback;
    StreamingCallbackType _streamingCallback;
    WriteCallbackType _writeCallback;
    CloseCallbackType _closeCallback;
    ConnectCallbackType _connectCallback;
    bool _connecting{false};
    int _state{S_NONE};
    int _pendingCallbacks{0};
    bool _closing{false};
    bool _closed{false};
};


class IOStream: public BaseIOStream {
public:
    IOStream(SocketType &&socket,
             Reactor *reactor,
             size_t maxBufferSize = DEFAULT_MAX_BUFFER_SIZE,
             size_t readChunkSize = DEFAULT_READ_CHUNK_SIZE)
            :BaseIOStream(std::move(socket), reactor, maxBufferSize, readChunkSize) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::IOStreamCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~IOStream() override {
        NET4CXX_Watcher->dec(WatchKeys::IOStreamCount);
    }
#endif

    void realConnect(const std::string &address, unsigned short port) override;
    void readFromSocket() override;
    void writeToSocket() override;
    void closeSocket() override;

    template <typename ...Args>
    static std::shared_ptr<IOStream> create(Args&& ...args) {
        return std::make_shared<IOStream>(std::forward<Args>(args)...);
    }
};


class SSLIOStream: public BaseIOStream {
public:
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> SSLSocketType;

    SSLIOStream(SocketType &&socket,
                SSLOptionPtr sslOption,
                Reactor *reactor,
                size_t maxBufferSize=DEFAULT_MAX_BUFFER_SIZE,
                size_t readChunkSize=DEFAULT_READ_CHUNK_SIZE)
            : BaseIOStream(std::move(socket), reactor, maxBufferSize, readChunkSize)
            , _sslOption(std::move(sslOption))
            , _sslSocket(_socket, _sslOption->context()) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::SSLIOStreamCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~SSLIOStream() override {
        NET4CXX_Watcher->dec(WatchKeys::SSLIOStreamCount);
    }
#endif

    void clearCallbacks() override;
    void realConnect(const std::string &address, unsigned short port) override;
    void readFromSocket() override;
    void writeToSocket() override;
    void closeSocket() override;

    template <typename ...Args>
    static std::shared_ptr<SSLIOStream> create(Args&& ...args) {
        return std::make_shared<SSLIOStream>(std::forward<Args>(args)...);
    }
protected:
    void doHandshake();
    void doRead();
    void doWrite();
    void doClose();
    void onHandshake(const boost::system::error_code &ec);
    void onShutdown(const boost::system::error_code &ec);

    SSLOptionPtr _sslOption;
    SSLSocketType _sslSocket;
    bool _sslAccepting{false};
    bool _sslAccepted{false};
    ConnectCallbackType _sslConnectCallback;
};

NS_END

#endif //NET4CXX_CORE_IOSTREAM_IOSTREAM_H
