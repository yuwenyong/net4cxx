//
// Created by yuwenyong.vincent on 2018/5/5.
//

#include "net4cxx/plugins/websocket/compress.h"


NS_BEGIN


const char* PerMessageDeflateConstants::EXTENSION_NAME = "permessage-deflate";

const std::vector<size_t> PerMessageDeflateConstants::WINDOW_SIZE_PERMISSIBLE_VALUES = {8, 9, 10, 11, 12, 13, 14, 15 };

const std::vector<int> PerMessageDeflateConstants::MEM_LEVEL_PERMISSIBLE_VALUES = {1, 2, 3, 4, 5, 6, 7, 8, 9 };


constexpr int PerMessageDeflate::DEFAULT_WINDOW_BITS;
constexpr int PerMessageDeflate::DEFAULT_MEM_LEVEL;

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
    data.erase(std::prev(data.end(), std::max(4ul, data.size())), data.end());
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
    return _decompressor->decompress(data, length);
}

void PerMessageDeflate::endDecompressMessage() {
    const ByteArray block = {0x00, 0x00, 0xff, 0xff};
    _decompressor->decompress(block);
}

std::string PerMessageDeflate::getExtensionName() const {
    return PerMessageDeflateConstants::EXTENSION_NAME;
}


std::string PerMessageDeflateOffer::getExtensionString() const {
    std::string pmce = PerMessageDeflateConstants::EXTENSION_NAME;
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

PtrMap<std::string, PerMessageCompressFactory> PERMESSAGE_COMPRESSION_EXTENSION;

NS_END