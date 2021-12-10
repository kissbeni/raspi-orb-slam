
#include <stdexcept>

#include "serializer.h"

// ----------------------- //
//  Boolean serialization  //
// ----------------------- //

/* static */
void Serializer::serialize(const bool b, std::vector<uint8_t>& to) {
    to.push_back(b ? 1 : 0);
}
/* static */
void Serializer::deserialize(bool& b, VectorStream& from) {
    b = !!from.get();
}

// -------------------- //
//  int8 serialization  //
// -------------------- //

/* static */
void Serializer::serialize(const int8_t num, std::vector<uint8_t>& to) {
    to.push_back(static_cast<uint8_t>(num));
}
/* static */
void Serializer::deserialize(int8_t& num, VectorStream& from) {
    num = from.get();
}

// --------------------- //
//  uint8 serialization  //
// --------------------- //

/* static */
void Serializer::serialize(const uint8_t num, std::vector<uint8_t>& to) {
    to.push_back(num);
}
/* static */
void Serializer::deserialize(uint8_t& num, VectorStream& from) {
    num = from.get();
}

// ----------------------- //
//  integer serialization  //
// ----------------------- //

/* static */
void Serializer::serialize(uint64_t num, std::vector<uint8_t>& to) {
    do {
        uint8_t bits = num & 0x7F;
        num >>= 7;
        to.push_back(bits | ((num != 0)*0x80));
    } while (num != 0);
}
/* static */
void Serializer::deserialize(uint64_t& num, VectorStream& from) {
    num = 0;
    int shift = 0;
    uint8_t b;
    do {
        if (shift >= (1 << 8))
            throw std::out_of_range("varint too long");

        b = from.get();
        num |= (b & 0x7F) << shift;
        shift += 7;
    } while ((b & 0x80) != 0);
}

/*static*/
void Serializer::serialize(uint16_t num, std::vector<uint8_t>& to) {
    serialize(static_cast<uint64_t>(num) & 0xFFFFull, to);
}
/*static*/
void Serializer::deserialize(uint16_t& num, VectorStream& from) {
    uint64_t tmp;
    deserialize(tmp, from);
    num = static_cast<uint16_t>(tmp);
}

// ---------------------- //
//  string serialization  //
// ---------------------- //

/* static */
void Serializer::serialize(const std::string& str, std::vector<uint8_t>& to) {
    serialize(static_cast<uint64_t>(str.length()), to);
    to.resize(to.size() + str.size());
    std::copy(str.begin(), str.end(), to.end() - str.size());
}
/* static */
void Serializer::deserialize(std::string& str, VectorStream& from) {
    uint64_t len;
    deserialize(len, from);

    str.resize(len);
    std::copy(from.curr(), from.curr() + len, str.begin());
    from.advance(len);
}

// --------------------- //
//  float serialization  //
// --------------------- //

/* static */
void Serializer::serialize(const float num, std::vector<uint8_t>& to) {
    uint32_t uf = *reinterpret_cast<const uint32_t*>(&num);
    uint8_t tmp[4];
    tmp[0] = uf & 0xFF;
    tmp[1] = (uf >> 8) & 0xFF;
    tmp[2] = (uf >> 16) & 0xFF;
    tmp[3] = (uf >> 24) & 0xFF;
    to.insert(to.end(), tmp, tmp + 4);
}
/* static */
void Serializer::deserialize(float& num, VectorStream& from) {
    uint32_t uf = 0;
    uf |= from.get();
    uf |= ((uint32_t)from.get()) << 8;
    uf |= ((uint32_t)from.get()) << 16;
    uf |= ((uint32_t)from.get()) << 24;
    num = *reinterpret_cast<float*>(&uf);
}

// -------------------- //
//  vec2 serialization  //
// -------------------- //

/* static */
void Serializer::serialize(const vec2 vec, std::vector<uint8_t>& to) {
    // TODO
    serialize(vec.x, to);
    serialize(vec.y, to);
}
/* static */
void Serializer::deserialize(vec2& vec, VectorStream& from) {
    // TODO
    deserialize(vec.x, from);
    deserialize(vec.y, from);
}

// -------------------- //
//  vec3 serialization  //
// -------------------- //

/* static */
void Serializer::serialize(const vec3 vec, std::vector<uint8_t>& to) {
    uint64_t ux = *reinterpret_cast<const uint32_t*>(&vec.x);
    uint64_t uy = *reinterpret_cast<const uint32_t*>(&vec.y);
    uint64_t uz = *reinterpret_cast<const uint32_t*>(&vec.z);

    uint8_t ex = ((ux >> 23) & 0xFF) - 127;
    uint8_t ey = ((uy >> 23) & 0xFF) - 127;
    uint8_t ez = ((uz >> 23) & 0xFF) - 127;

    uint64_t res = 0;

    res |= static_cast<uint64_t>(!!(ux & (1 << 31))) << 63; // sign of X
    res |= static_cast<uint64_t>(ex & 0x1F) << 58;          // exponent of X
    res |= ((ux >> 8) & 0x7fff) << 43;                      // mantissa of X

    res |=static_cast<uint64_t>(!!(uy & (1 << 31))) << 42;  // sign of Y
    res |= static_cast<uint64_t>(ey & 0x1F) << 37;          // exponent of Y
    res |= ((uy >> 8) & 0x7fff) << 22;                      // mantissa of Y

    res |= static_cast<uint64_t>(!!(uz & (1 << 31))) << 21; // sign of Z
    res |= static_cast<uint64_t>(ez & 0x1F) << 16;          // exponent of Z
    res |= ((uz >> 8) & 0x7fff) << 1;                       // mantissa of Z

    // Rounding bit
    res |= (!!(ux & (1 << 16)) + !!(uy & (1 << 16)) + !!(uz & (1 << 16)) >= 2);

    uint8_t tmp[8];
    tmp[0] = res & 0xFF;
    tmp[1] = (res >> 8) & 0xFF;
    tmp[2] = (res >> 16) & 0xFF;
    tmp[3] = (res >> 24) & 0xFF;
    tmp[4] = (res >> 32) & 0xFF;
    tmp[5] = (res >> 40) & 0xFF;
    tmp[6] = (res >> 48) & 0xFF;
    tmp[7] = (res >> 56) & 0xFF;
    to.insert(to.end(), tmp, tmp + 8);
}
/* static */
void Serializer::deserialize(vec3& vec, VectorStream& from) {
    uint64_t in = 0;
    in |= from.get();
    in |= ((uint64_t)from.get()) << 8;
    in |= ((uint64_t)from.get()) << 16;
    in |= ((uint64_t)from.get()) << 24;
    in |= ((uint64_t)from.get()) << 32;
    in |= ((uint64_t)from.get()) << 40;
    in |= ((uint64_t)from.get()) << 48;
    in |= ((uint64_t)from.get()) << 56;

    uint8_t roundingBit = in & 1;

    uint32_t temp;
    temp = 0;
    temp |= static_cast<uint32_t>(!!(in & (1ull << 63))) << 31;                             // sign of X
    temp |= (((0xe0ul << 5)*(in & (1ull << 62)) | ((in >> 58)& 0x1full)) + 127) << 23;      // exponent of X
    temp |= (0xff*roundingBit) | (((in >> 43) & 0x7fffull) << 8);                           // mantissa of X
    vec.x = *reinterpret_cast<float*>(&temp);

    temp = 0;
    temp |= static_cast<uint32_t>(!!(in & (1ull << 42))) << 31;                             // sign of Y
    temp |= (((0xe0ul << 5)*(in & (1ull << 41)) | ((in >> 37)& 0x1full)) + 127) << 23;      // exponent of Y
    temp |= (0xff*roundingBit) | (((in >> 22) & 0x7fffull) << 8);                           // mantissa of Y
    vec.y = *reinterpret_cast<float*>(&temp);

    temp = 0;
    temp |= static_cast<uint32_t>(!!(in & (1ull << 21))) << 31;                             // sign of Y
    temp |= ((((0xe0ul)*!!(in & (1ull << 20)) | ((in >> 16)& 0x1full)) + 127)&0xff) << 23;  // exponent of Y
    temp |= (0xff*roundingBit) | (((in >> 1) & 0x7fffull) << 8);                            // mantissa of Y
    vec.z = *reinterpret_cast<float*>(&temp);
}

// ---------------------- //
//  Object serialization  //
// ---------------------- //

/* static */
void Serializer::serialize(const Serializable* obj, std::vector<uint8_t>& to) {
    auto tmp = obj->serialize();
    size_t ogsize = to.size();
    to.resize(ogsize + tmp.size());
    std::copy(tmp.begin(), tmp.end(), to.begin() + ogsize);
}

/* static */
void Serializer::deserialize(Serializable* obj, VectorStream& from) {
    if (obj == nullptr) {
        throw std::runtime_error("cannot deserialize stream into a null pointer");
    }

    size_t oldShadowOffset = from.setShadowOffset(from.mPosition);
    obj->deserialize(from);
    from.setShadowOffset(oldShadowOffset);
}

