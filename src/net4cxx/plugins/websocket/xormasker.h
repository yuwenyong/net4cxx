//
// Created by yuwenyong.vincent on 2018/5/19.
//

#ifndef NET4CXX_PLUGINS_WEBSOCKET_XORMASKER_H
#define NET4CXX_PLUGINS_WEBSOCKET_XORMASKER_H

#include "net4cxx/plugins/websocket/base.h"

NS_BEGIN


class XorMasker {
public:
    virtual uint64_t pointer() const = 0;
    virtual void reset() = 0;
    virtual void process(Byte *data, uint64_t len) = 0;
    virtual ~XorMasker() = default;
};


class XorMaskerNull: public XorMasker {
public:
    XorMaskerNull()
            : _ptr(0) {

    }

    uint64_t pointer() const override;

    void reset() override;

    void process(Byte *data, uint64_t len) override;
protected:
    uint64_t _ptr;
};


class XorMaskerSimple: public XorMasker {
public:
    XorMaskerSimple(const WebSocketMask &mask)
            : _ptr(0)
            , _msk(mask) {

    }

    uint64_t pointer() const override;

    void reset() override;

    void process(Byte *data, uint64_t len) override;
protected:
    uint64_t _ptr;
    WebSocketMask _msk;
};


class XorMaskerShifted1: public XorMasker {
public:
    XorMaskerShifted1(const WebSocketMask &mask)
            : _ptr(0) {
        for (size_t j = 0; j < mask.size(); ++j) {
            _mskarray[0][j] = mask[j & 3u];
            _mskarray[1][j] = mask[(j + 1) & 3u];
            _mskarray[2][j] = mask[(j + 2) & 3u];
            _mskarray[3][j] = mask[(j + 3) & 3u];
        }
    }

    uint64_t pointer() const override;

    void reset() override;

    void process(Byte *data, uint64_t len) override;
protected:
    uint64_t _ptr;
    std::array<WebSocketMask, 4> _mskarray;
};


inline std::unique_ptr<XorMasker> createXorMasker(const WebSocketMask &mask, uint64_t length=0) {
    if (length < 128) {
        return std::make_unique<XorMaskerSimple>(mask);
    } else {
        return std::make_unique<XorMaskerShifted1>(mask);
    }
}

NS_END

#endif //NET4CXX_PLUGINS_WEBSOCKET_XORMASKER_H
