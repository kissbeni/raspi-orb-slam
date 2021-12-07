
#include "packets.h"
#include "serailizer.h"

std::vector<uint8_t> PingPacket::serialize() const {
    _SERIALIZE_BEGIN
    _SERIALIZE_FIELD(mData)
    _SERIALIZE_RETURN
}
void PingPacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mData)
}


std::vector<uint8_t> PongPacket::serialize() const {
    _SERIALIZE_BEGIN
    _SERIALIZE_FIELD(mData)
    _SERIALIZE_RETURN
}
void PongPacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mData)
}


std::vector<uint8_t> MovePacket::serialize() const {
    _SERIALIZE_BEGIN
    _SERIALIZE_FIELD(mLeftSpeed)
    _SERIALIZE_FIELD(mRightSpeed)
    _SERIALIZE_RETURN
}
void MovePacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mLeftSpeed)
    _DESERIALIZE_FIELD(mRightSpeed)
}

std::vector<uint8_t> StopPacket::serialize() const {
    _SERIALIZE_BEGIN
    _SERIALIZE_RETURN
}
void StopPacket::deserialize(VectorStream& from) {}


LedColor::LedColor(uint8_t r, uint8_t g, uint8_t b)
    : mRed{r}, mGreen{g}, mBlue{b} {}
std::vector<uint8_t> LedColor::serialize() const {
    _SERIALIZE_BEGIN
    _SERIALIZE_FIELD(mRed)
    _SERIALIZE_FIELD(mGreen)
    _SERIALIZE_FIELD(mBlue)
    _SERIALIZE_RETURN
}
void LedColor::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mRed)
    _DESERIALIZE_FIELD(mGreen)
    _DESERIALIZE_FIELD(mBlue)
}


std::vector<uint8_t> LedsPacket::serialize() const {
    _SERIALIZE_BEGIN
    _SERIALIZE_FIELD(mLeds)
    _SERIALIZE_RETURN
}
void LedsPacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mLeds)
}


std::vector<uint8_t> ReportPacket::serialize() const {
    _SERIALIZE_BEGIN
    _SERIALIZE_FIELD(mFps)
    _SERIALIZE_FIELD(mOverlay)
    _SERIALIZE_FIELD(mWorldPoints)
    _SERIALIZE_RETURN
}
void ReportPacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mFps)
    _DESERIALIZE_FIELD(mOverlay)
    _DESERIALIZE_FIELD(mWorldPoints)
}
