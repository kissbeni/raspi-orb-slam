
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <stdint.h>

#include "serailizer.h"
#include "packets.h"

void dumpVector(const std::vector<uint8_t>& data) {
    for (uint8_t b : data) {
        printf("%02x ", b);
    }
    puts("");
}

int main(int argc, char const *argv[])
{
    std::vector<uint8_t> test;
    Serializer::serialize("Hello world!", test);
    Serializer::serialize("", test);
    Serializer::serialize("Almafa", test);
    Serializer::serialize("", test);
    dumpVector(test);

    VectorStream vs{test};
    std::string s1,s2,s3,s4;
    Serializer::deserialize(s1, vs);
    Serializer::deserialize(s2, vs);
    Serializer::deserialize(s3, vs);
    Serializer::deserialize(s4, vs);
    std::cout << "|" << s1 << "|" << std::endl;
    std::cout << "|" << s2 << "|" << std::endl;
    std::cout << "|" << s3 << "|" << std::endl;
    std::cout << "|" << s4 << "|" << std::endl;

    PingPacket pkt;
    pkt.mData = "The quick brown fox jumps over the lazy dog!";
    auto res = pkt.serialize();

    dumpVector(res);

    VectorStream vs1{res};

    PingPacket pkt2;
    pkt2.deserialize(vs1);
    std::cout << pkt2.mData << std::endl;

    MovePacket mpkt;
    mpkt.mLeftSpeed = -100;
    mpkt.mRightSpeed = 0;
    auto res2 = mpkt.serialize();

    dumpVector(res2);

    VectorStream vs2{res2};

    MovePacket mpkt2;
    mpkt2.deserialize(vs2);
    std::cout << (int)mpkt2.mLeftSpeed << ", " << (int)mpkt2.mRightSpeed << std::endl;

    LedsPacket lpkt;
    lpkt.mLeds.push_back(std::make_unique<LedColor>(255, 0, 120));
    lpkt.mLeds.push_back(std::make_unique<LedColor>(0, 0, 255));
    lpkt.mLeds.push_back(std::make_unique<LedColor>(255, 255, 255));
    auto res3 = lpkt.serialize();

    dumpVector(res3);

    vec3 testvec3;
    testvec3.x = 69.4258;
    testvec3.y = 12345;
    testvec3.z = 3.141592654;
    std::vector<uint8_t> res4;
    Serializer::serialize(testvec3, res4);
    dumpVector(res4);

    VectorStream vs3{res4};
    vec3 testvec3_t;
    Serializer::deserialize(testvec3_t, vs3);
    printf("%.4f, %.4f, %.4f\n", testvec3_t.x, testvec3_t.y, testvec3_t.z);

    return 0;
}
