//
// Created by yuwenyong on 17-9-15.
//

#ifndef NET4CXX_COMMON_CRYPTO_BASE64_H
#define NET4CXX_COMMON_CRYPTO_BASE64_H

#include "net4cxx/common/common.h"

NS_BEGIN

class Base64 {
public:
    static std::string b64encode(const std::string &s, const char *altChars= nullptr);
    static ByteArray b64encode(const ByteArray &s, const char *altChars= nullptr);
    static std::string b64decode(const std::string &s, const char *altChars= nullptr);
    static ByteArray b64decode(const ByteArray &s, const char *altChars= nullptr);
};

NS_END

#endif //NET4CXX_COMMON_CRYPTO_BASE64_H
