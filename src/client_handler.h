
#ifndef _WS_CLIENT_HANDLER
#define _WS_CLIENT_HANDLER

#include <http.hpp>
#include "protocol/vector_stream.h"
#include "protocol/packets.h"

struct TankProtocolHandler : public WebsockClientHandler {
    void onConnect() override {
        puts("Connect!");
    }

    void onBinaryMessage(const std::vector<uint8_t>& data) override {
        char packetObjectMem[getMaximumPacketObjectSize()];

        switch (data[0] >> 5)
        {
            case PacketOpcode::MOVE:
                handlePacket(deserializePacket<MovePacket>(packetObjectMem, data)); break;
            case PacketOpcode::STOP:
                handlePacket(deserializePacket<StopPacket>(packetObjectMem, data)); break;
            default:
                puts("Received invalid packet");
                break;
        }
    }

    void onDisconnect() override {
        puts("Disconnect!");
    }

    private:
        template<typename T>
        T* deserializePacket(void* mem, const std::vector<uint8_t>& data) {
            auto res = new (mem) T;
            res->deserialize(data);
            return res;
        }

        void handlePacket(MovePacket* pkt);
};

#endif
