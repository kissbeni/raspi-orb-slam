
#ifndef _PROTO_VECTOR_STREAM_H
#define _PROTO_VECTOR_STREAM_H

#include <vector>
#include <cstdint>
#include <cstddef>

#include "protocol_types.h"

struct VectorStream {
    const std::vector<uint8_t>& mData;
    size_t mPosition;

    VectorStream(const std::vector<uint8_t>& vec);
    uint8_t operator[](size_t idx) const;
    uint8_t get();
    std::vector<uint8_t>::const_iterator curr() const;
    void advance(size_t n);
    size_t setShadowOffset(size_t newOffset) noexcept;

    private:
        size_t mShadowOffset = 0;
};

#endif
