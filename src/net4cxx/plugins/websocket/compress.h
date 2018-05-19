//
// Created by yuwenyong.vincent on 2018/5/5.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H
#define NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H

#include "net4cxx/plugins/websocket/base.h"


NS_BEGIN

class PerMessageCompressOffer {
public:
    virtual std::string getExtensionString() const = 0;
    virtual ~PerMessageCompressOffer();
};

using PerMessageCompressOfferPtr = std::shared_ptr<PerMessageCompressOffer>;


class PerMessageCompressOfferAccept {
public:
    virtual ~PerMessageCompressOfferAccept();
};

using PerMessageCompressOfferAcceptPtr = std::shared_ptr<PerMessageCompressOfferAccept>;


class PerMessageCompressResponse {
public:
    virtual ~PerMessageCompressResponse();
};

using PerMessageCompressResponsePtr = std::shared_ptr<PerMessageCompressResponse>;


class PerMessageCompressResponseAccept {
public:
    virtual ~PerMessageCompressResponseAccept();
};

using PerMessageCompressResponseAcceptPtr = std::shared_ptr<PerMessageCompressResponseAccept>;


class PerMessageCompress {
public:
    virtual std::string getExtensionName() const = 0;
    virtual ~PerMessageCompress();
};

using PerMessageCompressPtr = std::shared_ptr<PerMessageCompress>;

using PerMessageCompressionAccept4Server = std::function<
        PerMessageCompressOfferAcceptPtr (std::vector<PerMessageCompressOfferPtr>)>;

using PerMessageCompressionAccept4Client = std::function<
        PerMessageCompressResponseAcceptPtr (PerMessageCompressResponsePtr)>;


class PerMessageDeflate {
public:
    static const char * EXTENSION_NAME;
    static const std::vector<size_t> WINDOW_SIZE_PERMISSIBLE_VALUES;
    static const std::vector<int> MEM_LEVEL_PERMISSIBLE_VALUES;
};


class PerMessageDeflateOffer: public PerMessageCompressOffer {
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

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H
