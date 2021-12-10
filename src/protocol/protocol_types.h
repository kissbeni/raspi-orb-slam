
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
  MOVE = 1, // Movement with speed
  STOP = 2, // Immediate stop
  LEDS = 3, // Update LED colors
  STAT = 4, // System statistics
  OPUD = 5, // Overlay and point update
  CPUD = 6, // Camera path update
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
