//
// Created by yuwenyong.vincent on 2018/5/5.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H
#define NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H

#include "net4cxx/plugins/websocket/base.h"
#include "net4cxx/common/compress/zlib.h"
#include "net4cxx/common/utilities/util.h"


NS_BEGIN

class NET4CXX_COMMON_API PerMessageCompressOffer {
public:
    virtual std::string getExtensionName() const = 0;
    virtual std::string getExtensionString() const = 0;
    virtual ~PerMessageCompressOffer() = default;
};

using PerMessageCompressOfferPtr = std::shared_ptr<PerMessageCompressOffer>;


class NET4CXX_COMMON_API PerMessageCompressOfferAccept {
public:
    virtual std::string getExtensionName() const = 0;
    virtual std::string getExtensionString() const = 0;
    virtual ~PerMessageCompressOfferAccept() = default;
};

using PerMessageCompressOfferAcceptPtr = std::shared_ptr<PerMessageCompressOfferAccept>;


class NET4CXX_COMMON_API PerMessageCompressResponse {
public:
    virtual std::string getExtensionName() const = 0;
    virtual ~PerMessageCompressResponse() = default;
};

using PerMessageCompressResponsePtr = std::shared_ptr<PerMessageCompressResponse>;


class NET4CXX_COMMON_API PerMessageCompressResponseAccept {
public:
    virtual std::string getExtensionName() const = 0;
    virtual ~PerMessageCompressResponseAccept() = default;
};

using PerMessageCompressResponseAcceptPtr = std::shared_ptr<PerMessageCompressResponseAccept>;


class NET4CXX_COMMON_API PerMessageCompress {
public:
    virtual void startCompressMessage() = 0;
    virtual ByteArray compressMessageData(const Byte *data, size_t length) = 0;
    virtual ByteArray endCompressMessage() = 0;
    virtual void startDecompressMessage() = 0;
    virtual ByteArray decompressMessageData(const Byte *data, size_t length) = 0;
    virtual void endDecompressMessage() = 0;
    virtual std::string getExtensionName() const = 0;
    virtual ~PerMessageCompress()= default;
};

using PerMessageCompressPtr = std::shared_ptr<PerMessageCompress>;

using PerMessageCompressionAccept4Server = std::function<
        PerMessageCompressOfferAcceptPtr (std::vector<PerMessageCompressOfferPtr>)>;

using PerMessageCompressionAccept4Client = std::function<
        PerMessageCompressResponseAcceptPtr (PerMessageCompressResponsePtr)>;


class NET4CXX_COMMON_API PerMessageCompressFactory {
public:
    virtual PerMessageCompressOfferPtr createOfferFromParams(const WebSocketExtensionParams &params) = 0;
    virtual PerMessageCompressResponsePtr createResponseFromParams(const WebSocketExtensionParams &params) = 0;
    virtual PerMessageCompressPtr createFromOfferAccept(bool isServer, PerMessageCompressOfferAcceptPtr accept) = 0;
    virtual PerMessageCompressPtr createFromResponseAccept(bool isServer, PerMessageCompressResponseAcceptPtr accept) = 0;
    virtual ~PerMessageCompressFactory() = default;
};


class PerMessageDeflateConstants {
public:
    static const char * EXTENSION_NAME;
    static const std::vector<size_t> WINDOW_SIZE_PERMISSIBLE_VALUES;
    static const std::vector<int> MEM_LEVEL_PERMISSIBLE_VALUES;
};


class NET4CXX_COMMON_API PerMessageDeflateOffer: public PerMessageCompressOffer {
public:
    explicit PerMessageDeflateOffer(bool acceptNoContextTakeover=true,
                                    bool acceptMaxWindowBits=true,
                                    bool requestNoContextTakeover=false,
                                    int requestMaxWindowBits=0)
            : _acceptNoContextTakeover(acceptNoContextTakeover)
            , _acceptMaxWindowBits(acceptMaxWindowBits)
            , _requestNoContextTakeover(requestNoContextTakeover)
            , _requestMaxWindowBits(requestMaxWindowBits) {

    }

    std::string getExtensionName() const override;

    std::string getExtensionString() const override;

    bool getAcceptNoContextTakeover() const {
        return _acceptNoContextTakeover;
    }

    bool getAcceptMaxWindowBits() const {
        return _acceptMaxWindowBits;
    }

    bool getRequestNoContextTakeover() const {
        return _requestNoContextTakeover;
    }

    int getRequestMaxWindowBits() const {
        return _requestMaxWindowBits;
    }
protected:
    bool _acceptNoContextTakeover;
    bool _acceptMaxWindowBits;
    bool _requestNoContextTakeover;
    int _requestMaxWindowBits;
};

using PerMessageDeflateOfferPtr = std::shared_ptr<PerMessageDeflateOffer>;


class NET4CXX_COMMON_API PerMessageDeflateOfferAccept: public PerMessageCompressOfferAccept {
public:
    explicit PerMessageDeflateOfferAccept(PerMessageCompressOfferPtr offer,
                                          bool requestNoContextTakeover=false,
                                          int requestMaxWindowBits=0,
                                          boost::optional<bool> noContextTakeover=boost::none,
                                          boost::optional<int> windowBits=boost::none,
                                          boost::optional<int> memLevel=boost::none,
                                          boost::optional<size_t> maxMessageSize=boost::none);

    std::string getExtensionName() const override;

    std::string getExtensionString() const override;

    PerMessageDeflateOfferPtr getOffer() const {
        return _offer;
    }

    bool getRequestNoContextTakeover() const {
        return _requestNoContextTakeover;
    }

    int getRequestMaxWindowBits() const {
        return _requestMaxWindowBits;
    }

    boost::optional<bool> getNoContextTakeover() const {
        return _noContextTakeover;
    }

    boost::optional<int> getWindowBits() const {
        return _windowBits;
    }

    boost::optional<int> getMemLevel() const {
        return _memLevel;
    }

    boost::optional<size_t> getMaxMessageSize() const {
        return _maxMessageSize;
    }
protected:
    PerMessageDeflateOfferPtr _offer;
    bool _requestNoContextTakeover;
    int _requestMaxWindowBits;
    boost::optional<bool> _noContextTakeover;
    boost::optional<int> _windowBits;
    boost::optional<int> _memLevel;
    boost::optional<size_t> _maxMessageSize;
};

using PerMessageDeflateOfferAcceptPtr = std::shared_ptr<PerMessageDeflateOfferAccept>;


class NET4CXX_COMMON_API PerMessageDeflateResponse: public PerMessageCompressResponse {
public:
    PerMessageDeflateResponse(int clientMaxWindowBits, bool clientNoContextTakeover, int serverMaxWindowBits,
                              bool serverNoContextTakeover)
            : _clientMaxWindowBits(clientMaxWindowBits)
            , _clientNoContextTakeover(clientNoContextTakeover)
            , _serverMaxWindowBits(serverMaxWindowBits)
            , _serverNoContextTakeover(serverNoContextTakeover) {

    }

    std::string getExtensionName() const override;

    int getClientMaxWindowBits() const {
        return _clientMaxWindowBits;
    }

    int getServerMaxWindowBits() const {
        return _serverMaxWindowBits;
    }

    bool getClientNoContextTakeover() const {
        return _clientNoContextTakeover;
    }

    bool getServerNoContextTakeover() const {
        return _serverNoContextTakeover;
    }
protected:
    int _clientMaxWindowBits;
    bool _clientNoContextTakeover;
    int _serverMaxWindowBits;
    bool _serverNoContextTakeover;
};

using PerMessageDeflateResponsePtr = std::shared_ptr<PerMessageDeflateResponse>;


class NET4CXX_COMMON_API PerMessageDeflateResponseAccept: public PerMessageCompressResponseAccept {
public:
    explicit PerMessageDeflateResponseAccept(PerMessageCompressResponsePtr response,
                                             boost::optional<bool> noContextTakeover=boost::none,
                                             boost::optional<int> windowBits=boost::none,
                                             boost::optional<int> memLevel=boost::none,
                                             boost::optional<size_t> maxMessageSize=boost::none);

    std::string getExtensionName() const override;

    PerMessageDeflateResponsePtr getResponse() const {
        return _response;
    }

    boost::optional<bool> getNoContextTakeover() const {
        return _noContextTakeover;
    }

    boost::optional<int> getWindowBits() const {
        return _windowBits;
    }

    boost::optional<int> getMemLevel() const {
        return _memLevel;
    }

    boost::optional<size_t> getMaxMessageSize() const {
        return _maxMessageSize;
    }
protected:
    PerMessageDeflateResponsePtr _response;
    boost::optional<bool> _noContextTakeover;
    boost::optional<int> _windowBits;
    boost::optional<int> _memLevel;
    boost::optional<size_t> _maxMessageSize;
};

using PerMessageDeflateResponseAcceptPtr = std::shared_ptr<PerMessageDeflateResponseAccept>;


class NET4CXX_COMMON_API PerMessageDeflate: public PerMessageCompress {
public:
    static constexpr int DEFAULT_WINDOW_BITS = Zlib::maxWBits;
    static constexpr int DEFAULT_MEM_LEVEL = 8;

    PerMessageDeflate(bool isServer,
                      bool serverNoContextTakeover,
                      bool clientNoContextTakeover,
                      int serverMaxWindowBits,
                      int clientMaxWindowBits,
                      boost::optional<int> memLevel,
                      boost::optional<size_t> maxMessageSize)
            : _isServer(isServer)
            , _serverNoContextTakeover(serverNoContextTakeover)
            , _clientNoContextTakeover(clientNoContextTakeover)
            , _serverMaxWindowBits(serverMaxWindowBits ? serverMaxWindowBits : DEFAULT_WINDOW_BITS)
            , _clientMaxWindowBits(clientMaxWindowBits ? clientMaxWindowBits : DEFAULT_WINDOW_BITS)
            , _memLevel(memLevel && *memLevel ? *memLevel : DEFAULT_MEM_LEVEL)
            , _maxMessageSize(maxMessageSize) {

    }

    std::string getExtensionName() const override;

    void startCompressMessage() override;

    ByteArray compressMessageData(const Byte *data, size_t length) override;

    ByteArray endCompressMessage() override;

    void startDecompressMessage() override;

    ByteArray decompressMessageData(const Byte *data, size_t length) override;

    void endDecompressMessage() override;
protected:
    bool _isServer;
    bool _serverNoContextTakeover;
    bool _clientNoContextTakeover;
    int _serverMaxWindowBits;
    int _clientMaxWindowBits;
    int _memLevel;
    boost::optional<size_t> _maxMessageSize;
    std::unique_ptr<CompressObj> _compressor;
    std::unique_ptr<DecompressObj> _decompressor;
};

using PerMessageDeflatePtr = std::shared_ptr<PerMessageDeflate>;


class NET4CXX_COMMON_API PerMessageDeflateFactory: public PerMessageCompressFactory {
public:
    PerMessageCompressOfferPtr createOfferFromParams(const WebSocketExtensionParams &params) override;

    PerMessageCompressResponsePtr createResponseFromParams(const WebSocketExtensionParams &params) override;

    PerMessageCompressPtr createFromOfferAccept(bool isServer, PerMessageCompressOfferAcceptPtr accept) override;

    PerMessageCompressPtr createFromResponseAccept(bool isServer, PerMessageCompressResponseAcceptPtr accept) override;
};


extern PtrMap<std::string, PerMessageCompressFactory> PERMESSAGE_COMPRESSION_EXTENSION;

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H
