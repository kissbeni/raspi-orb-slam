
#include "client_handler.h"

void TankProtocolHandler::onConnect() {
    puts("Connect!");
    mAllClients.insert(this);
}

void TankProtocolHandler::onBinaryMessage(const std::vector<uint8_t>& data) {
    char packetObjectMem[getMaximumPacketObjectSize()];

    switch (data[0] >> 5)
    {
        case PacketOpcode::MOVE:
            packetHandlers::handlePacket(deserializePacket<MovePacket>(packetObjectMem, data)); break;
        case PacketOpcode::STOP:
            packetHandlers::handlePacket(deserializePacket<StopPacket>(packetObjectMem, data)); break;
        case PacketOpcode::LEDS:
            packetHandlers::handlePacket(deserializePacket<LedsPacket>(packetObjectMem, data)); break;
        default:
            puts("Received invalid packet");
            break;
    }
}

void TankProtocolHandler::onDisconnect() {
    puts("Disconnect!");
    mAllClients.erase(this);
}

/*static*/ void TankProtocolHandler::sendToAll(const Serializable& ser) {
    for (const auto& c : mAllClients)
        c->sendBinary(ser.serialize());
}
