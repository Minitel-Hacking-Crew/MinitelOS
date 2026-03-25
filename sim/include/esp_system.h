#pragma once
#include <cstdlib>

inline void esp_restart() {
    // In sim, restart = exit
    exit(0);
}
