//
// Created by yuwenyong on 17-9-14.
//

#ifndef NET4CXX_COMMON_CRYPTO_HASHLIB_H
#define NET4CXX_COMMON_CRYPTO_HASHLIB_H

#include "net4cxx/common/common.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <boost/mpl/string.hpp>


#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x00908000)
#define _OPENSSL_SUPPORTS_SHA2
#endif


#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(LIBRESSL_VERSION_NUMBER)
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

NS_BEGIN

class NET4CXX_COMMON_API EVPContext {
public:
    explicit EVPContext(const char *name);

    ~EVPContext();

    const EVP_MD_CTX* native() const {
        return _ctx;
    }
protected:
    EVP_MD_CTX *_ctx{nullptr};
};


template <typename HashNameT>
class EVPObject {
public:
    typedef HashNameT HashNameType;

    EVPObject() {
        _ctx = EVP_MD_CTX_new();
        EVP_MD_CTX_copy(_ctx, initializer().native());
    }

    EVPObject(const Byte *d, size_t cnt): EVPObject() {
        update(d, cnt);
    }

    explicit EVPObject(const ByteArray &data): EVPObject() {
        update(data);
    }

    explicit EVPObject(const char *s): EVPObject() {
        update(s);
    }

    explicit EVPObject(const std::string &data): EVPObject() {
        update(data);
    }

    EVPObject(const EVPObject &rhs) {
        _ctx = EVP_MD_CTX_new();
        EVP_MD_CTX_copy(_ctx, rhs._ctx);
    }

    EVPObject(EVPObject &&rhs) noexcept {
        _ctx = rhs._ctx;
        rhs._ctx = nullptr;
    }

    EVPObject& operator=(const EVPObject &rhs) {
        if (this == &rhs) {
            return *this;
        }
        reset();
        _ctx = EVP_MD_CTX_new();
        EVP_MD_CTX_copy(_ctx, rhs._ctx);
        return *this;
    }

    EVPObject& operator=(EVPObject &&rhs) noexcept {
        reset();
        _ctx = rhs._ctx;
        rhs._ctx = nullptr;
        return *this;
    }

    ~EVPObject() {
        reset();
    }

    void update(const Byte *d, size_t cnt) {
        EVP_DigestUpdate(_ctx, d, cnt);
    }

    void update(const ByteArray &data) {
        update(data.data(), data.size());
    }

    void update(const char *s) {
        update((const Byte*)s, strlen(s));;
    }

    void update(const std::string &data) {
        update((const Byte*)data.data(), data.size());
    }

    int digestSize() const {
        return EVP_MD_CTX_size(_ctx);
    }

    int blockSize() const {
        return EVP_MD_CTX_block_size(_ctx);
    }

    std::string digest() const {
        unsigned char digest[EVP_MAX_MD_SIZE];
        std::string result;
        unsigned int digestSize;
        EVPObject temp(*this);
        digestSize = (unsigned int)temp.digestSize();
        EVP_DigestFinal(temp._ctx, digest, NULL);
        result.assign((char *)digest, digestSize);
        return result;
    }

    std::string hex() const {
        unsigned char digest[EVP_MAX_MD_SIZE];
        std::string result;
        unsigned int digestSize;
        EVPObject temp(*this);
        digestSize = (unsigned int)temp.digestSize();
        EVP_DigestFinal(temp._ctx, digest, NULL);
        char c;
        for(unsigned int i = 0; i < digestSize; i++) {
            c = (char)((digest[i] >> 4) & 0xf);
            c = (char)((c > 9) ? c +'a'-10 : c + '0');
            result.push_back(c);
            c = (char)(digest[i] & 0xf);
            c = (char)((c > 9) ? c +'a'-10 : c + '0');
            result.push_back(c);
        }
        return result;
    }
protected:
    void reset() {
        if (_ctx) {
            EVP_MD_CTX_free(_ctx);
            _ctx = nullptr;
        }
    }

    static const EVPContext& initializer() {
        static const EVPContext context(boost::mpl::c_str<HashNameType>::value);
        return context;
    }

    EVP_MD_CTX *_ctx{nullptr};
};


typedef EVPObject<boost::mpl::string<'m', 'd', '5'>> MD5Object;
typedef EVPObject<boost::mpl::string<'s', 'h', 'a', '1'>> SHA1Object;
#ifdef _OPENSSL_SUPPORTS_SHA2
typedef EVPObject<boost::mpl::string<'s', 'h', 'a', '2', '2', '4'>> SHA224Object;
typedef EVPObject<boost::mpl::string<'s', 'h', 'a', '2', '5', '6'>> SHA256Object;
typedef EVPObject<boost::mpl::string<'s', 'h', 'a', '3', '8', '4'>> SHA384Object;
typedef EVPObject<boost::mpl::string<'s', 'h', 'a', '5', '1', '2'>> SHA512Object;
#endif

NS_END

#endif //NET4CXX_COMMON_CRYPTO_HASHLIB_H
