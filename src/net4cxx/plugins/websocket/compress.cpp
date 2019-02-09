//
// Created by yuwenyong.vincent on 2018/5/5.
//

#include "net4cxx/plugins/websocket/compress.h"


NS_BEGIN


const char* PerMessageDeflateConstants::EXTENSION_NAME = "permessage-deflate";

const std::vector<size_t> PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES = {8, 9, 10, 11, 12, 13, 14, 15 };

const std::vector<int> PerMessageDeflateConstants::MEM_LEVEL_PERMISSIBLE_VALUES = {1, 2, 3, 4, 5, 6, 7, 8, 9 };


std::string PerMessageDeflateOffer::getExtensionName() const {
    return PerMessageDeflateConstants::EXTENSION_NAME;
}

std::string PerMessageDeflateOffer::getExtensionString() const {
    std::string pmce = getExtensionName();
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


PerMessageDeflateOfferAccept::PerMessageDeflateOfferAccept(PerMessageCompressOfferPtr offer,
                                                           bool requestNoContextTakeover,
                                                           int requestMaxWindowBits,
                                                           boost::optional<bool> noContextTakeover,
                                                           boost::optional<int> windowBits,
                                                           boost::optional<int> memLevel,
                                                           boost::optional<size_t> maxMessageSize) {

    _offer = std::dynamic_pointer_cast<PerMessageDeflateOffer>(offer);
    if (!_offer) {
        NET4CXX_THROW_EXCEPTION(Exception, "Invalid type for offer");
    }

    if (requestNoContextTakeover && !_offer->getAcceptNoContextTakeover()) {
        NET4CXX_THROW_EXCEPTION(Exception, "invalid value %s for request_no_context_takeover"
                                           " - feature unsupported by client",
                                TypeCast<std::string>(requestNoContextTakeover));
    }
    _requestNoContextTakeover = requestNoContextTakeover;

    if (requestMaxWindowBits != 0 &&
        !std::binary_search(PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.begin(),
                            PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.end(),
                            requestMaxWindowBits)) {
        NET4CXX_THROW_EXCEPTION(Exception, "invalid value %d for request_max_window_bits", requestMaxWindowBits);
    }

    if (requestMaxWindowBits != 0 && !_offer->getAcceptMaxWindowBits()) {
        NET4CXX_THROW_EXCEPTION(Exception, "invalid value %d for request_max_window_bits"
                                           " - feature unsupported by client",
                                requestMaxWindowBits);
    }
    _requestMaxWindowBits = requestMaxWindowBits;

    if (noContextTakeover) {
        if (_offer->getRequestNoContextTakeover() && !*noContextTakeover) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid value %s for no_context_takeover - client requested feature",
                                    TypeCast<std::string>(*noContextTakeover));
        }
    }
    _noContextTakeover = noContextTakeover;

    if (windowBits) {
        if (!std::binary_search(PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.begin(),
                                PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.end(), *windowBits)) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid value %d for window_bits", *windowBits);
        }

        if (_offer->getRequestMaxWindowBits() != 0 && *windowBits > _offer->getRequestMaxWindowBits()) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid value %d for window_bits"
                                               " - client requested lower maximum value",
                                    *windowBits);
        }
    }
    _windowBits = windowBits;

    if (memLevel) {
        if (!std::binary_search(PerMessageDeflateConstants::MEM_LEVEL_PERMISSIBLE_VALUES.begin(),
                                PerMessageDeflateConstants::MEM_LEVEL_PERMISSIBLE_VALUES.end(), *memLevel)) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid value %d for mem_level", *memLevel);
        }
    }
    _memLevel = memLevel;
    _maxMessageSize = maxMessageSize;
}

std::string PerMessageDeflateOfferAccept::getExtensionName() const {
    return PerMessageDeflateConstants::EXTENSION_NAME;
}

std::string PerMessageDeflateOfferAccept::getExtensionString() const {
    std::string pmce = getExtensionName();
    if (_offer->getRequestNoContextTakeover()) {
        pmce += "; server_no_context_takeover";
    }
    if (_offer->getRequestMaxWindowBits() != 0) {
        pmce += "; server_max_window_bits=" + std::to_string(_offer->getRequestMaxWindowBits());
    }
    if (_requestNoContextTakeover) {
        pmce += "; client_no_context_takeover";
    }
    if (_requestMaxWindowBits) {
        pmce += "; client_max_window_bits=" + std::to_string(_requestMaxWindowBits);
    }
    return pmce;
}


std::string PerMessageDeflateResponse::getExtensionName() const {
    return PerMessageDeflateConstants::EXTENSION_NAME;
}


PerMessageDeflateResponseAccept::PerMessageDeflateResponseAccept(PerMessageCompressResponsePtr response,
                                                                 boost::optional<bool> noContextTakeover,
                                                                 boost::optional<int> windowBits,
                                                                 boost::optional<int> memLevel,
                                                                 boost::optional<size_t> maxMessageSize) {
    _response = std::dynamic_pointer_cast<PerMessageDeflateResponse>(response);
    if (!_response) {
        NET4CXX_THROW_EXCEPTION(Exception, "Invalid type for response");
    }

    if (noContextTakeover) {
        if (_response->getClientNoContextTakeover() && !*noContextTakeover) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid value %s for no_context_takeover - server requested feature",
                                    TypeCast<std::string>(*noContextTakeover));
        }
    }
    _noContextTakeover = noContextTakeover;

    if (windowBits) {
        if (!std::binary_search(PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.begin(),
                                PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.end(), *windowBits)) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid value %d for window_bits", *windowBits);
        }

        if (_response->getClientMaxWindowBits() != 0 && *windowBits > _response->getClientMaxWindowBits()) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid value %d for window_bits"
                                               " - server requested lower maximum value",
                                    *windowBits);
        }
    }
    _windowBits = windowBits;

    if (memLevel) {
        if (!std::binary_search(PerMessageDeflateConstants::MEM_LEVEL_PERMISSIBLE_VALUES.begin(),
                                PerMessageDeflateConstants::MEM_LEVEL_PERMISSIBLE_VALUES.end(), *memLevel)) {
            NET4CXX_THROW_EXCEPTION(Exception, "invalid value %d for mem_level", *memLevel);
        }
    }
    _memLevel = memLevel;
    _maxMessageSize = maxMessageSize;
}

std::string PerMessageDeflateResponseAccept::getExtensionName() const {
    return PerMessageDeflateConstants::EXTENSION_NAME;
}


constexpr int PerMessageDeflate::DEFAULT_WINDOW_BITS;
constexpr int PerMessageDeflate::DEFAULT_MEM_LEVEL;


std::string PerMessageDeflate::getExtensionName() const {
    return PerMessageDeflateConstants::EXTENSION_NAME;
}

void PerMessageDeflate::startCompressMessage() {
    if (_isServer) {
        if (!_compressor || _serverNoContextTakeover) {
            _compressor = std::make_unique<CompressObj>(Zlib::zDefaultCompression, Zlib::deflated,
                                                        -_serverMaxWindowBits, _memLevel);
        }
    } else {
        if (!_compressor || _clientNoContextTakeover) {
            _compressor = std::make_unique<CompressObj>(Zlib::zDefaultCompression, Zlib::deflated,
                                                        -_clientMaxWindowBits, _memLevel);
        }
    }
}

ByteArray PerMessageDeflate::compressMessageData(const Byte *data, size_t length) {
    return _compressor->compress(data, length);
}

ByteArray PerMessageDeflate::endCompressMessage() {
    auto data = _compressor->flush(Zlib::zSyncFlush);
    data.erase(std::prev(data.end(), (ssize_t)std::max(4ul, (unsigned long)data.size())), data.end());
    return data;
}

void PerMessageDeflate::startDecompressMessage() {
    if (_isServer) {
        if (!_decompressor || _clientNoContextTakeover) {
            _decompressor = std::make_unique<DecompressObj>(-_clientMaxWindowBits);
        }
    } else {
        if (!_decompressor || _serverNoContextTakeover) {
            _decompressor = std::make_unique<DecompressObj>(-_serverMaxWindowBits);
        }
    }
}

ByteArray PerMessageDeflate::decompressMessageData(const Byte *data, size_t length) {
    if (_maxMessageSize) {
        return _decompressor->decompress(data, length, *_maxMessageSize);
    }
    return _decompressor->decompress(data, length);
}

void PerMessageDeflate::endDecompressMessage() {
    const ByteArray block = {0x00, 0x00, 0xff, 0xff};
    _decompressor->decompress(block);
}


PerMessageCompressOfferPtr PerMessageDeflateFactory::createOfferFromParams(const WebSocketExtensionParams &params) {
    bool acceptMaxWindowBits = false;
    bool acceptNoContextTakeover = true;
    int requestMaxWindowBits = 0;
    bool requestNoContextTakeover = false;

    for (auto &param: params) {
        auto &p = param.first;
        if (param.second.size() > 1) {
            NET4CXX_THROW_EXCEPTION(Exception, "multiple occurrence of extension parameter '%s' for extension '%s'", p,
                                    PerMessageDeflateConstants::EXTENSION_NAME);
        }

        auto &val = param.second.front();

        if (p == "client_max_window_bits") {
            if (val) {
                int value;
                try {
                    value = std::stoi(*val);
                } catch (...) {
                    NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                       "for parameter '%s' of extension '%s'", *val, p,
                                            PerMessageDeflateConstants::EXTENSION_NAME);
                }
                if (!std::binary_search(PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.begin(),
                                        PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.end(), value)) {
                    NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                       "for parameter '%s' of extension '%s'", *val, p,
                                            PerMessageDeflateConstants::EXTENSION_NAME);
                } else {
                    acceptMaxWindowBits = true;
                }
            } else {
                acceptMaxWindowBits = true;
            }
        } else if (p == "client_no_context_takeover") {
            if (val) {
                NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                   "for parameter '%s' of extension '%s'", *val, p,
                                        PerMessageDeflateConstants::EXTENSION_NAME);
            } else {
                acceptNoContextTakeover = true;
            }
        } else if (p == "server_max_window_bits") {
            if (val) {
                int value;
                try {
                    value = std::stoi(*val);
                } catch (...) {
                    NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                       "for parameter '%s' of extension '%s'", *val, p,
                                            PerMessageDeflateConstants::EXTENSION_NAME);
                }
                if (!std::binary_search(PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.begin(),
                                        PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.end(), value)) {
                    NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                       "for parameter '%s' of extension '%s'", *val, p,
                                            PerMessageDeflateConstants::EXTENSION_NAME);
                } else {
                    requestMaxWindowBits = value;
                }
            } else {
                NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                   "for parameter '%s' of extension '%s'", "", p,
                                        PerMessageDeflateConstants::EXTENSION_NAME);
            }
        } else if (p == "server_no_context_takeover") {
            if (val) {
                NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                   "for parameter '%s' of extension '%s'", *val, p,
                                        PerMessageDeflateConstants::EXTENSION_NAME);
            } else {
                requestNoContextTakeover = true;
            }
        } else {
            NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter '%s' for extension '%s'", p,
                                    PerMessageDeflateConstants::EXTENSION_NAME);
        }
    }
    return std::make_shared<PerMessageDeflateOffer>(acceptNoContextTakeover, acceptMaxWindowBits,
                                                    requestNoContextTakeover, requestMaxWindowBits);
}

PerMessageCompressResponsePtr PerMessageDeflateFactory::createResponseFromParams(
        const WebSocketExtensionParams &params) {
    int clientMaxWindowBits = 0;
    bool clientNoContextTakeover = false;
    int serverMaxWindowBits = 0;
    bool serverNoContextTakeover = false;

    for (auto &param: params) {
        auto &p = param.first;
        if (param.second.size() > 1) {
            NET4CXX_THROW_EXCEPTION(Exception, "multiple occurrence of extension parameter '%s' for extension '%s'", p,
                                    PerMessageDeflateConstants::EXTENSION_NAME);
        }

        auto &val = param.second.front();

        if (p == "client_max_window_bits") {
            if (val) {
                int value;
                try {
                    value = std::stoi(*val);
                } catch (...) {
                    NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                       "for parameter '%s' of extension '%s'", *val, p,
                                            PerMessageDeflateConstants::EXTENSION_NAME);
                }
                if (!std::binary_search(PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.begin(),
                                        PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.end(), value)) {
                    NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                       "for parameter '%s' of extension '%s'", *val, p,
                                            PerMessageDeflateConstants::EXTENSION_NAME);
                } else {
                    clientMaxWindowBits = value;
                }
            } else {
                NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                   "for parameter '%s' of extension '%s'", "", p,
                                        PerMessageDeflateConstants::EXTENSION_NAME);
            }
        } else if (p == "client_no_context_takeover") {
            if (val) {
                NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                   "for parameter '%s' of extension '%s'", *val, p,
                                        PerMessageDeflateConstants::EXTENSION_NAME);
            } else {
                clientNoContextTakeover = true;
            }
        } else if (p == "server_max_window_bits") {
            if (val) {
                int value;
                try {
                    value = std::stoi(*val);
                } catch (...) {
                    NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                       "for parameter '%s' of extension '%s'", *val, p,
                                            PerMessageDeflateConstants::EXTENSION_NAME);
                }
                if (!std::binary_search(PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.begin(),
                                        PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES.end(), value)) {
                    NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                       "for parameter '%s' of extension '%s'", *val, p,
                                            PerMessageDeflateConstants::EXTENSION_NAME);
                } else {
                    serverMaxWindowBits = value;
                }
            } else {
                NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                   "for parameter '%s' of extension '%s'", "", p,
                                        PerMessageDeflateConstants::EXTENSION_NAME);
            }
        } else if (p == "server_no_context_takeover") {
            if (val) {
                NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter value '%s' "
                                                   "for parameter '%s' of extension '%s'", *val, p,
                                        PerMessageDeflateConstants::EXTENSION_NAME);
            } else {
                serverNoContextTakeover = true;
            }
        } else {
            NET4CXX_THROW_EXCEPTION(Exception, "illegal extension parameter '%s' for extension '%s'", p,
                                    PerMessageDeflateConstants::EXTENSION_NAME);
        }
    }
    return std::make_shared<PerMessageDeflateResponse>(clientMaxWindowBits, clientNoContextTakeover,
                                                       serverMaxWindowBits, serverNoContextTakeover);
}

PerMessageCompressPtr PerMessageDeflateFactory::createFromResponseAccept(bool isServer,
                                                                         PerMessageCompressResponseAcceptPtr accept) {
    auto acceptor = std::static_pointer_cast<PerMessageDeflateResponseAccept>(accept);
    auto pmce = std::make_shared<PerMessageDeflate>(
            isServer,
            acceptor->getResponse()->getServerNoContextTakeover(),
            acceptor->getNoContextTakeover() ?
            *acceptor->getNoContextTakeover() : acceptor->getResponse()->getClientNoContextTakeover(),
            acceptor->getResponse()->getServerMaxWindowBits(),
            acceptor->getWindowBits() ?
            *acceptor->getWindowBits() : acceptor->getResponse()->getClientMaxWindowBits(),
            acceptor->getMemLevel(),
            acceptor->getMaxMessageSize());
    return pmce;
}

PerMessageCompressPtr PerMessageDeflateFactory::createFromOfferAccept(bool isServer,
                                                                      PerMessageCompressOfferAcceptPtr accept) {
    auto acceptor = std::static_pointer_cast<PerMessageDeflateOfferAccept>(accept);
    auto pmce = std::make_shared<PerMessageDeflate>(
            isServer,
            acceptor->getNoContextTakeover() ?
            *acceptor->getNoContextTakeover() : acceptor->getOffer()->getRequestNoContextTakeover(),
            acceptor->getRequestNoContextTakeover(),
            acceptor->getWindowBits() ?
            *acceptor->getWindowBits() : acceptor->getOffer()->getRequestMaxWindowBits(),
            acceptor->getRequestMaxWindowBits(),
            acceptor->getMemLevel(),
            acceptor->getMaxMessageSize());
    return pmce;
}


PtrMap<std::string, PerMessageCompressFactory> PERMESSAGE_COMPRESSION_EXTENSION =
        PtrMapCreator<std::string, PerMessageCompressFactory>()
                (PerMessageDeflateConstants::EXTENSION_NAME, std::make_unique<PerMessageDeflateFactory>())
                ();

NS_END