
#include "packets.h"
#include "serializer.h"

std::vector<uint8_t> MovePacket::serialize() const {
    _SERIALIZE_BEGIN_OPC
    _SERIALIZE_FIELD(mLeftSpeed)
    _SERIALIZE_FIELD(mRightSpeed)
    _SERIALIZE_RETURN
}
void MovePacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mLeftSpeed)
    _DESERIALIZE_FIELD(mRightSpeed)
}

std::vector<uint8_t> StopPacket::serialize() const {
    _SERIALIZE_BEGIN_OPC
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
    _SERIALIZE_BEGIN_OPC
    _SERIALIZE_FIELD(mLeds)
    _SERIALIZE_RETURN
}
void LedsPacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mLeds)
}


OverlayPoint::OverlayPoint(uint16_t x, uint16_t y, uint8_t f)
    : mX{x}, mY{y}, mFlags{f} {}

std::vector<uint8_t> OverlayPoint::serialize() const {
    _SERIALIZE_BEGIN
    _SERIALIZE_FIELD(mX)
    _SERIALIZE_FIELD(mY)
    _SERIALIZE_FIELD(mFlags)
    _SERIALIZE_RETURN
}
void OverlayPoint::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mX)
    _DESERIALIZE_FIELD(mY)
    _DESERIALIZE_FIELD(mFlags)
}


std::vector<uint8_t> ReportPacket::serialize() const {
    _SERIALIZE_BEGIN_OPC
    _SERIALIZE_FIELD(mOverlay)
    _SERIALIZE_FIELD(mWorldPoints)
    _SERIALIZE_RETURN
}
void ReportPacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mOverlay)
    _DESERIALIZE_FIELD(mWorldPoints)
}


std::vector<uint8_t> PathPacket::serialize() const {
    _SERIALIZE_BEGIN_OPC
    _SERIALIZE_FIELD(mCurrentCameraPos)
    _SERIALIZE_FIELD(mIndex)
    _SERIALIZE_RETURN
}
void PathPacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mCurrentCameraPos)
    _DESERIALIZE_FIELD(mIndex)
}


std::vector<uint8_t> MetricsPacket::serialize() const {
    _SERIALIZE_BEGIN_OPC
    _SERIALIZE_FIELD(mFps)
    _SERIALIZE_FIELD(mCpuUsage)
    _SERIALIZE_FIELD(mMemUsage)
    _SERIALIZE_FIELD(mTrackingState)
    _SERIALIZE_RETURN
}
void MetricsPacket::deserialize(VectorStream& from) {
    _DESERIALIZE_FIELD(mFps)
    _DESERIALIZE_FIELD(mCpuUsage)
    _DESERIALIZE_FIELD(mMemUsage)
    _DESERIALIZE_FIELD(mTrackingState)
}
