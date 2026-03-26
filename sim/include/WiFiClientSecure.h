#pragma once
#include "Arduino.h"

class WiFiClientSecure {
public:
    void setInsecure() {}
    bool connect(const char*, int) { return false; }
    void stop() {}
    bool connected() { return false; }
};
