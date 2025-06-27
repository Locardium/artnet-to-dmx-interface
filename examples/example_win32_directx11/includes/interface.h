#pragma once

#include "globals.h"

namespace Interface {
    int connect(int index);
    void disconnect();
    void start();
    void stop();
    void setRefreshRate(int value);
    void setDMX(const std::vector<uint8_t>& dmxData);
}
