
#include "client_handler.h"
#include <mutex>

/*static*/ std::set<TankProtocolHandler*> TankProtocolHandler::mAllClients;

std::mutex clientsMutex;

void TankProtocolHandler::onConnect() {
    puts("Connect!");
    std::lock_guard<std::mutex>{clientsMutex};
    mAllClients.insert(this);
}

void TankProtocolHandler::onBinaryMessage(const std::vector<uint8_t>& data) {
    char packetObjectMem[getMaximumPacketObjectSize()];

    switch (data[0])
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
    std::lock_guard<std::mutex>{clientsMutex};
    mAllClients.erase(this);
}

/*static*/ void TankProtocolHandler::sendToAll(const Serializable& ser) {
    //printf("Sending a packet to %d clients!\n", mAllClients.size());
    const auto data = ser.serialize();
    std::lock_guard<std::mutex>{clientsMutex};
    for (const auto& c : mAllClients)
        c->sendBinary(data);
}
