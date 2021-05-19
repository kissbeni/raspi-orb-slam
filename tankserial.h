
#ifndef TANKSERIAL_H
#define TANKSERIAL_H

#include <string>
#include <cstdint>

class TankSerial
{
    public:
        TankSerial(std::string port, unsigned baud = 115200);

        bool open();
        void close();

        ~TankSerial();

        bool stop();
        bool speed(int8_t left, int8_t right);
        bool setLedColor(uint8_t id, uint8_t r, uint8_t g, uint8_t b);
        bool setAllLeds(uint8_t r, uint8_t g, uint8_t b);
        bool clearLed(uint8_t id);
        bool clearLeds();
        bool setRainbowMode(bool state);
        bool ping();
    private:
        enum TankOpcode : uint8_t {
            TANKOPC_STOP    = 0x01,
            TANKOPC_MOVE    = 0x02,
            TANKOPC_LED_SET = 0x03,
            TANKOPC_LED_CLR = 0x04,
            TANKOPC_RAINBOW = 0x05,
            TANKOPC_PING    = 0x06
        };

        bool sendCommandRaw(const TankOpcode opc, std::initializer_list<uint8_t> args);

        std::string mPort;
        unsigned mBaud;
        int mPortFD = -1;
};

#endif
