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
    virtual ~PerMessageCompressResponse() = default;
};

using PerMessageCompressResponsePtr = std::shared_ptr<PerMessageCompressResponse>;


class NET4CXX_COMMON_API PerMessageCompressResponseAccept {
public:
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
    virtual PerMessageCompressPtr createFromReponseAccept(bool isServer, PerMessageCompressResponseAcceptPtr accept) = 0;
    virtual ~PerMessageCompressFactory() = default;
};


class PerMessageDeflateConstants {
public:
    static const char * EXTENSION_NAME;
    static const std::vector<size_t> WINDOW_SIZE_PERMISSIBLE_VALUES;
    static const std::vector<int> MEM_LEVEL_PERMISSIBLE_VALUES;
};


class NET4CXX_COMMON_API PerMessageDeflate: public PerMessageCompress {
public:
    static constexpr int DEFAULT_WINDOW_BITS = Zlib::maxWBits;
    static constexpr int DEFAULT_MEM_LEVEL = 8;

    PerMessageDeflate(bool isServer,
                      bool serverNoContextTakeover,
                      bool clientNoContextTakeover,
                      int serverMaxWindowBits,
                      int clientMaxWindowBits,
                      int memLevel)
            : _isServer(isServer)
            , _serverNoContextTakeover(serverNoContextTakeover)
            , _clientNoContextTakeover(clientNoContextTakeover)
            , _serverMaxWindowBits(serverMaxWindowBits ? serverMaxWindowBits : DEFAULT_WINDOW_BITS)
            , _clientMaxWindowBits(clientMaxWindowBits ? clientMaxWindowBits : DEFAULT_WINDOW_BITS)
            , _memLevel(memLevel ? memLevel : DEFAULT_MEM_LEVEL) {

    }

    void startCompressMessage() override;

    ByteArray compressMessageData(const Byte *data, size_t length) override;

    ByteArray endCompressMessage() override;

    void startDecompressMessage() override;

    ByteArray decompressMessageData(const Byte *data, size_t length) override;

    void endDecompressMessage() override;

    std::string getExtensionName() const override;
protected:
    bool _isServer;
    bool _serverNoContextTakeover;
    bool _clientNoContextTakeover;
    int _serverMaxWindowBits;
    int _clientMaxWindowBits;
    int _memLevel;
    std::unique_ptr<CompressObj> _compressor;
    std::unique_ptr<DecompressObj> _decompressor;
};


class NET4CXX_COMMON_API PerMessageDeflateOffer: public PerMessageCompressOffer {
public:
    PerMessageDeflateOffer(bool acceptNoContextTakeover, bool acceptMaxWindowBits, bool requestNoContextTakeover,
                           size_t requestMaxWindowBits)
            : _acceptNoContextTakeover(acceptNoContextTakeover)
            , _acceptMaxWindowBits(acceptMaxWindowBits)
            , _requestNoContextTakeover(requestNoContextTakeover)
            , _requestMaxWindowBits(requestMaxWindowBits) {

    }

    std::string getExtensionString() const override;
protected:
    bool _acceptNoContextTakeover;
    bool _acceptMaxWindowBits;
    bool _requestNoContextTakeover;
    size_t _requestMaxWindowBits;
};


extern PtrMap<std::string, PerMessageCompressFactory> PERMESSAGE_COMPRESSION_EXTENSION;

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H
