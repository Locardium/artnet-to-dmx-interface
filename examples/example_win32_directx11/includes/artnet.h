#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace ArtNet {
    static constexpr int ARTNET_PORT = 6454;
    void selectInterface(const std::string& ip);
    bool startReceiving();
    void stopReceiving();
    bool isReceiving();
    std::vector<uint8_t> getDMX(uint16_t universe);
}
