//
// Created by yuwenyong.vincent on 2018/5/5.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H
#define NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H

#include "net4cxx/plugins/websocket/base.h"

NS_BEGIN

class PerMessageCompressOffer {
public:
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
    virtual ~PerMessageCompress();
};

using PerMessageCompressPtr = std::shared_ptr<PerMessageCompress>;

using PerMessageCompressionAccept4Server = std::function<
        PerMessageCompressOfferAcceptPtr (std::vector<PerMessageCompressOfferPtr>)>;

using PerMessageCompressionAccept4Client = std::function<
        PerMessageCompressResponseAcceptPtr (PerMessageCompressResponsePtr)>;

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_COMPRESS_H
