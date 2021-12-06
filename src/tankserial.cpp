
#include "tankserial.h"
#include <stdexcept>

// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

TankSerial::TankSerial(std::string port, unsigned baud)
    : mPort{port}, mBaud{baud} {}

TankSerial::~TankSerial()
{
    this->close();
}

bool TankSerial::open() {
    if (mPortFD > 0)
        this->close();

    mPortFD = ::open(mPort.c_str(), O_RDWR);

    if (mPortFD < 0) {
        fprintf(stderr, "TankSerial failed to open: %i (%s)\n", errno, strerror(errno));
        return false;
    }

    struct termios tty;

    if (tcgetattr(mPortFD, &tty) != 0) {
        fprintf(stderr, "TankSerial failed to get termios: %i (%s)\n", errno, strerror(errno));
        return false;
    }

    // No parity bit
    tty.c_cflag &= ~PARENB;

    // One stop bit
    tty.c_cflag &= ~CSTOPB;

    // 8 bits ber byte
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // No flow control
    tty.c_cflag &= ~CRTSCTS;

    // Disable canonical mode
    tty.c_lflag &= ~ICANON;

    // Set baud rate
    cfsetispeed(&tty, mBaud);

    if (tcsetattr(mPortFD, TCSANOW, &tty) != 0) {
        fprintf(stderr, "TankSerial failed to set termios: %i (%s)\n", errno, strerror(errno));
        return false;
    }

    return true;
}

void TankSerial::close() {
    if (mPortFD > 0)
    {
        ::close(mPortFD);
        mPortFD = -1;
    }
}

bool TankSerial::stop() {
    return sendCommandRaw(TANKOPC_STOP, {});
}

bool TankSerial::speed(int8_t left, int8_t right) {
    return sendCommandRaw(TANKOPC_MOVE, {
        static_cast<uint8_t>(left),
        static_cast<uint8_t>(right)
    });
}

bool TankSerial::setLedColor(uint8_t id, uint8_t r, uint8_t g, uint8_t b) {
    return sendCommandRaw(TANKOPC_LED_SET, {
        id,
        r,
        g,
        b
    });
}

bool TankSerial::setAllLeds(uint8_t r, uint8_t g, uint8_t b) {
    bool success = true;

    for (uint8_t i = 0; i < 8; i++)
        success &= setLedColor(i, r, g, b);

    return success;
}

bool TankSerial::clearLed(uint8_t id) {
    return sendCommandRaw(TANKOPC_LED_CLR, {id});
}

bool TankSerial::clearLeds() {
    return sendCommandRaw(TANKOPC_LED_CLR, {});
}

bool TankSerial::setRainbowMode(bool state) {
    return sendCommandRaw(TANKOPC_RAINBOW, { state });
}

bool TankSerial::ping() {
    return sendCommandRaw(TANKOPC_PING, {});
}

bool TankSerial::sendCommandRaw(const TankOpcode opc, std::initializer_list<uint8_t> args)
{
    uint8_t packet[10] = {};

    if (args.size() > 5)
        throw std::out_of_range("Too many command arguments");

    packet[0] = 0x18; // ASCII CAN (cancel)
    packet[1] = 0x11; // ASCII DC1 (device control 1)
    packet[2] = opc | (args.size() << 4);

    std::copy(args.begin(), args.end(), &packet[3]);

    uint8_t checksum = 0;
    for (int i = 2; i < 8; i++)
        checksum += packet[i];

    packet[8] = -checksum;
    packet[9] = 0x17; // ASCII ETB (end of transmission block)

    printf("Serial packet: ");
    for (int i = 0; i < 10; i++)
        printf("%02x", ((unsigned)packet[i])&0xFF);
    puts("");

    if (write(mPortFD, packet, 10) != 10)
    {
        fprintf(stderr, "Failed to send packet\n");
        return false;
    }

    return true;

    uint8_t b;

    // Read response byte, and ignore the prompt
    do { read(mPortFD, &b, 1); } while (b == '>');

    if (b == 0x06 /* ASCII ACK (acknowledgement) */)
        return read(mPortFD, &b, 1) == 1;

    if (b == 0x15 /* ASCII NAK (negative acknowledgement) */ )
    {
        read(mPortFD, &b, 1);
        fprintf(stderr, "Received error from when sending a packet: %02x\n", b);
        return false;
    }

    fprintf(stderr, "Received garbage from when sending a packet: %02x\n", b);
    return false;
}
