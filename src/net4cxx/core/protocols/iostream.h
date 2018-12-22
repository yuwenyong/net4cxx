//
// Created by yuwenyong.vincent on 2018/10/13.
//

#ifndef NET4CXX_CORE_PROTOCOLS_IOSTREAM_H
#define NET4CXX_CORE_PROTOCOLS_IOSTREAM_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"
#include "net4cxx/shared/global/constants.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(StreamClosedError, IOError);
NET4CXX_DECLARE_EXCEPTION(UnsatisfiableReadError, Exception);
NET4CXX_DECLARE_EXCEPTION(StreamBufferFullError, Exception);


class NET4CXX_COMMON_API IOStream: public Protocol, public std::enable_shared_from_this<IOStream> {
public:
    explicit IOStream(size_t maxBufferSize=0)
            : _maxBufferSize(maxBufferSize ? maxBufferSize : DEFAULT_MAX_BUFFER_SIZE) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(WatchKeys::IOStreamCount);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~IOStream() override {
        NET4CXX_Watcher->dec(WatchKeys::IOStreamCount);
    }
#endif

    void connectionMade() final;

    void dataReceived(Byte *data, size_t length) final;

    void connectionLost(std::exception_ptr reason) final;

    virtual void onConnected();

    virtual void onDataRead(Byte *data, size_t length) = 0;

    virtual void onWriteComplete();

    virtual void onDisconnected(std::exception_ptr reason);

    void readUntilRegex(const std::string &regex, size_t maxBytes=0);

    void readUntil(std::string delimiter, size_t maxBytes=0);

    void readBytes(size_t numBytes);

    void readUntilClose();

    void write(const Byte *data, size_t length, bool writeCallback=false);

    void write(const ByteArray &data, bool writeCallback=false) {
        write(data.data(), data.size(), writeCallback);
    }

    void write(const char *data, bool writeCallback=false) {
        write((const Byte *)data, strlen(data), writeCallback);
    }

    void write(const std::string &data, bool writeCallback=false) {
        write((const Byte *)data.c_str(), data.size(), writeCallback);
    }

    bool reading() const {
        return _readBytes || _readDelimiter || _readRegex || _readUntilClose;
    }

    virtual void close(std::exception_ptr error);

    template <typename SelfT>
    std::shared_ptr<const SelfT> getSelf() const {
        return std::static_pointer_cast<SelfT>(shared_from_this());
    }

    template <typename SelfT>
    std::shared_ptr<SelfT> getSelf() {
        return std::static_pointer_cast<SelfT>(shared_from_this());
    }

    static constexpr size_t DEFAULT_MAX_BUFFER_SIZE = 104857600;
protected:
    class WriteDoneTrigger: public Producer {
    public:
        explicit WriteDoneTrigger(const std::shared_ptr<IOStream> &stream): _stream(stream) {}

        void resumeProducing() override {
            auto stream = _stream.lock();
            if (stream) {
                stream->writeDone();
            }
        }
    protected:
        std::weak_ptr<IOStream> _stream;
    };

    void writeDone() {
        if (_writeCallback) {
            _writeCallback = false;
            onWriteComplete();
        }
    }

    void tryInlineRead();

    void readFromBuffer();

    bool findReadPos() const;

    void checkMaxBytes(const std::string &delimiter, size_t size) const;


    size_t _maxBufferSize;
    std::exception_ptr _error;
    MessageBuffer _readBuffer;
    boost::optional<std::string> _readDelimiter;
    boost::optional<boost::regex> _readRegex;
    boost::optional<size_t> _readMaxBytes;
    boost::optional<size_t> _readBytes;
    bool _readUntilClose{false};
    bool _writeCallback{false};
};


NS_END

#endif //NET4CXX_CORE_PROTOCOLS_IOSTREAM_H
