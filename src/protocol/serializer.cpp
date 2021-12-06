
#include <stdexcept>

#include "serailizer.h"

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

// ---------------------- //
//  string serialization  //
// ---------------------- //

/* static */
void Serializer::serialize(const std::string& str, std::vector<uint8_t>& to) {
    serialize(str.length(), to);
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
    // TODO
    serialize(vec.x, to);
    serialize(vec.y, to);
    serialize(vec.z, to);
}
/* static */
void Serializer::deserialize(vec3& vec, VectorStream& from) {
    // TODO
    deserialize(vec.x, from);
    deserialize(vec.y, from);
    deserialize(vec.z, from);
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

