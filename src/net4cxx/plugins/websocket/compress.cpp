//
// Created by yuwenyong.vincent on 2018/5/5.
//

#include "net4cxx/plugins/websocket/compress.h"


NS_BEGIN

PerMessageCompressOffer::~PerMessageCompressOffer() {

}


PerMessageCompressOfferAccept::~PerMessageCompressOfferAccept() {

}


PerMessageCompressResponse::~PerMessageCompressResponse() {

}


PerMessageCompressResponseAccept::~PerMessageCompressResponseAccept() {

}


PerMessageCompress::~PerMessageCompress() {

}

const char* PerMessageDeflate::EXTENSION_NAME = "permessage-deflate";

const std::vector<size_t> PerMessageDeflate::WINDOW_SIZE_PERMISSIBLE_VALUES = {8, 9, 10, 11, 12, 13, 14, 15 };

const std::vector<int> PerMessageDeflate::MEM_LEVEL_PERMISSIBLE_VALUES = {1, 2, 3, 4, 5, 6, 7, 8, 9 };


std::string PerMessageDeflateOffer::getExtensionString() const {
    std::string pmce = PerMessageDeflate::EXTENSION_NAME;
    if (_acceptNoContextTakeover) {
        pmce += "; client_no_context_takeover";
    }
    if (_acceptMaxWindowBits) {
        pmce += "; client_max_window_bits";
    }
    if (_requestNoContextTakeover) {
        pmce += "; server_no_context_takeover";
    }
    if (_requestMaxWindowBits != 0) {
        pmce += "; server_max_window_bits=" + std::to_string(_requestMaxWindowBits);
    }
    return pmce;
}

NS_END