
#ifndef _PROTO_TYPES_H
#define _PROTO_TYPES_H

#include <vector>
#include <cstdint>
#include <stdexcept>

struct VectorStream;

struct vec3 {
    float x, y, z;
};

struct vec2 {
    float x, y;
};

enum PacketOpcode : uint8_t {
  PING = 0, // Echo request
  PONG = 1, // Echo response
  MOVE = 2, // Movement with speed
  STOP = 3, // Immediate stop
  MODE = 4, // ?
  LEDS = 5, // Update LED colors
  STAT = 6, // System statistics
  PPUD = 7, // Path and point update
};

struct Serializable {
    virtual std::vector<uint8_t> serialize() const {
        throw std::runtime_error("This object cannot be serialized");
    }
    virtual void deserialize(VectorStream& from) {
        throw std::runtime_error("This object cannot be deserialized");
    }
};

template<PacketOpcode opcode>
struct BasePacket : virtual Serializable {
    virtual ~BasePacket() = default;
    constexpr uint8_t getOpcode() const noexcept { return opcode; }
};

#endif
