
#ifndef _WS_CLIENT_HANDLER
#define _WS_CLIENT_HANDLER

#include <http.hpp>
#include <set>
#include "protocol/vector_stream.h"
#include "protocol/packets.h"

namespace packetHandlers {
    void handlePacket(MovePacket* pkt);
    void handlePacket(StopPacket* pkt);
    void handlePacket(LedsPacket* pkt);
}

struct TankProtocolHandler : public WebsockClientHandler {
    void onConnect() override;
    void onBinaryMessage(const std::vector<uint8_t>& data) override;
    void onDisconnect() override;

    template<PacketOpcode opcode>
    static void sendToAll(const BasePacket<opcode>& pkt) {
        sendToAll(pkt);
    }

    private:
        template<typename T>
        T* deserializePacket(void* mem, const std::vector<uint8_t>& data) {
            auto res = new (mem) T;
            res->deserialize(data);
            return res;
        }

        static std::set<TankProtocolHandler*> mAllClients;
        static void sendToAll(const Serializable& ser);
};

#endif
