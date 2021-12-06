
#include "vector_stream.h"

VectorStream::VectorStream(const std::vector<uint8_t>& vec)
    : mData{vec}, mPosition{0} {}

uint8_t VectorStream::operator[](size_t idx) const {
    return mData[mShadowOffset + idx];
}

uint8_t VectorStream::get() {
    return mData[mShadowOffset + mPosition++];
}

std::vector<uint8_t>::const_iterator VectorStream::curr() const {
    return mData.begin() + mShadowOffset + mPosition;
}

void VectorStream::advance(size_t n) {
    mPosition += n;
}

size_t VectorStream::setShadowOffset(size_t offset) noexcept {
    std::swap(mShadowOffset, offset);
    mPosition -= mShadowOffset - offset;
    return offset;
}
