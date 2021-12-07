
#ifndef _PROTO_PACKETS_H
#define _PROTO_PACKETS_H

#include <memory>

#include "protocol_types.h"
#include "../utils.h"

struct PingPacket : BasePacket<PacketOpcode::PING> {
    std::string mData;

    virtual std::vector<uint8_t> serialize() const override;
    virtual void deserialize(VectorStream& from) override;
};

struct PongPacket : BasePacket<PacketOpcode::PONG> {
    std::string mData;

    virtual std::vector<uint8_t> serialize() const override;
    virtual void deserialize(VectorStream& from) override;
};

struct MovePacket : BasePacket<PacketOpcode::MOVE> {
    int8_t mLeftSpeed, mRightSpeed;

    virtual std::vector<uint8_t> serialize() const override;
    virtual void deserialize(VectorStream& from) override;
};

struct StopPacket : BasePacket<PacketOpcode::STOP> {
    virtual std::vector<uint8_t> serialize() const override;
    virtual void deserialize(VectorStream& from) override;
};

struct LedColor : Serializable {
    uint8_t mRed, mGreen, mBlue;

    LedColor() = default;
    LedColor(uint8_t r, uint8_t g, uint8_t b);

    virtual std::vector<uint8_t> serialize() const override;
    virtual void deserialize(VectorStream& from) override;
};

struct LedsPacket : BasePacket<PacketOpcode::LEDS> {
    std::vector<std::unique_ptr<LedColor>> mLeds;

    virtual std::vector<uint8_t> serialize() const override;
    virtual void deserialize(VectorStream& from) override;
};

struct OverlayPoint : Serializable {
    uint16_t x, y;
    uint8_t flags;

    OverlayPoint() = default;
    OverlayPoint(uint16_t x, uint16_t y, uint8_t f);

    virtual std::vector<uint8_t> serialize() const override;
    virtual void deserialize(VectorStream& from) override;
};

struct ReportPacket : BasePacket<PacketOpcode::PPUD> {
    float mFps;
    std::vector<OverlayPoint> mOverlay;
    std::vector<vec3> mWorldPoints;

    virtual std::vector<uint8_t> serialize() const override;
    virtual void deserialize(VectorStream& from) override;
};

static constexpr size_t getMaximumPacketObjectSize() noexcept {
    return (3 + vmax(
        sizeof(PingPacket),
        sizeof(PongPacket),
        sizeof(MovePacket),
        sizeof(LedsPacket)
    ) / 4) * 4;
}

#endif
