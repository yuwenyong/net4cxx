//
// Created by yuwenyong on 17-9-15.
//

#ifndef NET4CXX_COMMON_COMPRESS_ZLIB_H
#define NET4CXX_COMMON_COMPRESS_ZLIB_H

#include "net4cxx/common/common.h"
#include <zlib.h>
#include "net4cxx/common/utilities/errors.h"


#define DEFLATED   8
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
#define DEF_WBITS MAX_WBIT
#define DEFAULTALLOC (16*1024)


NS_BEGIN

NET4CXX_DECLARE_EXCEPTION(ZlibError, Exception);


class NET4CXX_COMMON_API Zlib {
public:
    static unsigned long adler32(const Byte *data, size_t len, unsigned long value=1) {
        return ::adler32(value, (Bytef *)data, (unsigned int)len);
    }

    static unsigned long adler32(const ByteArray &data, unsigned long value=1) {
        return adler32(data.data(), data.size(), value);
    }

    static unsigned long adler32(const char *data, unsigned long value=1) {
        return adler32((const Byte *)data, strlen(data), value);
    }

    static unsigned long adler32(const std::string &data, unsigned long value=1) {
        return adler32((const Byte *)data.data(), data.size(), value);
    }

    static unsigned long crc32(const Byte *data, size_t len, unsigned long value=0) {
        return ::crc32(value, (const Bytef *)data, (unsigned int)len);
    }

    static unsigned long crc32(const ByteArray &data, unsigned long value=0) {
        return crc32(data.data(), data.size(), value);
    }

    static unsigned long crc32(const char *data, unsigned long value=0) {
        return crc32((const Byte *)data, strlen(data), value);
    }

    static unsigned long crc32(const std::string &data, unsigned long value=0) {
        return crc32((const Byte *)data.data(), data.size(), value);
    }

    static ByteArray compress(const Byte *data, size_t len, int level=zDefaultCompression);

    static ByteArray compress(const ByteArray &data, int level=zDefaultCompression) {
        return compress(data.data(), data.size(), level);
    }

    static ByteArray compress(const char *data, int level=zDefaultCompression) {
        return compress((const Byte *)data, strlen(data), level);
    }

    static ByteArray compress(const std::string &data, int level=zDefaultCompression) {
        return compress((const Byte *)data.data(), data.size(), level);
    }

    static std::string compressToString(const Byte *data, size_t len, int level=zDefaultCompression);

    static std::string compressToString(const ByteArray &data, int level=zDefaultCompression) {
        return compressToString(data.data(), data.size(), level);
    }

    static std::string compressToString(const char* data, int level=zDefaultCompression) {
        return compressToString((const Byte *)data, strlen(data), level);
    }

    static std::string compressToString(const std::string &data, int level=zDefaultCompression) {
        return compressToString((const Byte *)data.data(), data.length(), level);
    }

    static ByteArray decompress(const Byte *data, size_t len, int wbits=maxWBits);

    static ByteArray decompress(const ByteArray &data, int wbits=maxWBits) {
        return decompress(data.data(), data.size(), wbits);
    }

    static ByteArray decompress(const std::string &data, int wbits=maxWBits) {
        return decompress((const Byte *)data.data(), data.size(), wbits);
    }

    static std::string decompressToString(const Byte *data, size_t len, int wbits=maxWBits);

    static std::string decompressToString(const ByteArray &data, int wbits=maxWBits) {
        return decompressToString(data.data(), data.size(), wbits);
    }

    static std::string decompressToString(const std::string &data, int wbits=maxWBits) {
        return decompressToString((const Byte *)data.data(), data.size(), wbits);
    }

    static void handleError(z_stream zst, int err, const char *msg);

    static constexpr int maxWBits = MAX_WBITS;
    static constexpr int deflated = DEFLATED;
    static constexpr int defMemLevel = DEF_MEM_LEVEL;
    static constexpr int zBestSpeed = Z_BEST_SPEED;
    static constexpr int zBestCompression = Z_BEST_COMPRESSION;
    static constexpr int zDefaultCompression = Z_DEFAULT_COMPRESSION;
    static constexpr int zFiltered = Z_FILTERED;
    static constexpr int zHuffmanOnly = Z_HUFFMAN_ONLY;
    static constexpr int zDefaultStrategy = Z_DEFAULT_STRATEGY;

    static constexpr int zFinish = Z_FINISH;
    static constexpr int zNoFlush = Z_NO_FLUSH;
    static constexpr int zSyncFlush = Z_SYNC_FLUSH;
    static constexpr int zFullFlush = Z_FULL_FLUSH;
};


class CompressObj {
public:
    explicit CompressObj(int level=Zlib::zDefaultCompression, int method=Zlib::deflated, int wbits=Zlib::maxWBits,
                         int memLevel=Zlib::defMemLevel, int strategy=Zlib::zDefaultStrategy);

    CompressObj(const CompressObj &rhs);

    CompressObj(CompressObj &&rhs) noexcept ;

    CompressObj& operator=(const CompressObj &rhs);

    CompressObj& operator=(CompressObj &&rhs) noexcept;

    ~CompressObj() {
        clear();
    }

    ByteArray compress(const Byte *data, size_t len);

    ByteArray compress(const ByteArray &data) {
        return compress(data.data(), data.size());
    }

    ByteArray compress(const char *data) {
        return compress((const Byte *)data, strlen(data));
    }

    ByteArray compress(const std::string &data) {
        return compress((const Byte *)data.data(), data.size());
    }

    std::string compressToString(const Byte *data, size_t len);

    std::string compressToString(const ByteArray &data) {
        return compressToString(data.data(), data.size());
    }

    std::string compressToString(const char *data) {
        return compressToString((const Byte *)data, strlen(data));
    }

    std::string compressToString(const std::string &data) {
        return compressToString((const Byte *)data.data(), data.size());
    }

    ByteArray flush(int flushMode=Zlib::zFinish);

    std::string flushToString(int flushMode=Zlib::zFinish);
protected:
    void initialize();

    void clear();

    bool _inited;
    int _level;
    int _method;
    int _wbits;
    int _memLevel;
    int _strategy;
    z_stream _zst;
};


class DecompressObj {
public:
    explicit DecompressObj(int wbits=Zlib::maxWBits);

    DecompressObj(const DecompressObj &rhs);

    DecompressObj(DecompressObj &&rhs) noexcept;

    DecompressObj& operator=(const DecompressObj &rhs);

    DecompressObj& operator=(DecompressObj &&rhs) noexcept;

    ~DecompressObj() {
        clear();
    }

    ByteArray decompress(const Byte *data, size_t len, size_t maxLength=0);

    ByteArray decompress(const ByteArray &data, size_t maxLength=0) {
        return decompress(data.data(), data.size(), maxLength);
    }

    ByteArray decompress(const std::string &data, size_t maxLength=0) {
        return decompress((const Byte *)data.data(), data.size(), maxLength);
    }

    std::string decompressToString(const Byte *data, size_t len, size_t maxLength=0);

    std::string decompressToString(const ByteArray &data, size_t maxLength=0) {
        return decompressToString(data.data(), data.size(), maxLength);
    }

    std::string decompressToString(const std::string &data, size_t maxLength=0) {
        return decompressToString((const Byte *)data.data(), data.size(), maxLength);
    }

    ByteArray flush();

    std::string flushToString();

    const ByteArray& getUnusedData() const {
        return _unusedData;
    }

    const ByteArray& getUnconsumedTail() const {
        return _unconsumedTail;
    }
protected:
    void saveUnconsumedInput(const Byte *data, size_t len, int err);

    void initialize();

    void clear();

    bool _inited;
    int _wbits;
    z_stream _zst;
    ByteArray _unusedData;
    ByteArray _unconsumedTail;
};

NS_END

#endif //NET4CXX_COMMON_COMPRESS_ZLIB_H
