//
// Created by yuwenyong on 17-9-14.
//

#include "net4cxx/common/crypto/hashlib.h"
#include "net4cxx/common/debugging/assert.h"


NS_BEGIN

class OpenSSLInit {
public:
    OpenSSLInit() noexcept {
        OpenSSL_add_all_digests();
        ERR_load_crypto_strings();
    }

    ~OpenSSLInit() = default;
};


OpenSSLInit gInit;


EVPContext::EVPContext(const char *name) {
    const EVP_MD *type = EVP_get_digestbyname(name);
    BOOST_ASSERT(type != nullptr);
    _ctx = EVP_MD_CTX_new();
    EVP_DigestInit(_ctx, type);
}


EVPContext::~EVPContext() {
    if (_ctx) {
        EVP_MD_CTX_free(_ctx);
        _ctx = nullptr;
    }
}

NS_END