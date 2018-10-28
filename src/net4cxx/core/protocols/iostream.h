//
// Created by yuwenyong.vincent on 2018/10/13.
//

#ifndef NET4CXX_CORE_PROTOCOLS_IOSTREAM_H
#define NET4CXX_CORE_PROTOCOLS_IOSTREAM_H

#include "net4cxx/common/common.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/core/network/protocol.h"
#include "net4cxx/core/network/reactor.h"
#include "net4cxx/shared/global/constants.h"


NS_BEGIN


NET4CXX_DECLARE_EXCEPTION(StreamClosedError, IOError);


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

    void dataReceived(Byte *data, size_t length) override;

    void connectionLost(std::exception_ptr reason) override;

    virtual void dataRead(Byte *data, size_t length) = 0;

    virtual void connectionClose(std::exception_ptr reason);

    void readUntilRegex(const std::string &regex);

    void readUntil(std::string delimiter);

    void readBytes(size_t numBytes);

    void readUntilClose();

    bool reading() const {
        return _readBytes || _readDelimiter || _readRegex || _readUntilClose;
    }

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
    void tryInlineRead() {
        reactor()->addCallback([this, self=shared_from_this()]() {
            readFromBuffer();
        });
    }

    void readFromBuffer();

    size_t _maxBufferSize;
    MessageBuffer _readBuffer;
    boost::optional<std::string> _readDelimiter;
    boost::optional<boost::regex> _readRegex;
    boost::optional<size_t> _readBytes;
    bool _readUntilClose{false};
};


NS_END

#endif //NET4CXX_CORE_PROTOCOLS_IOSTREAM_H
