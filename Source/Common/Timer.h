#pragma once

#include <chrono>
#include "Definitions.h"

/* Static class representing high resolution timer on Windows */
class HighResTimer {
public:
    HighResTimer() = delete;
    RULE_OF_ZERO(HighResTimer);
    using rep        = long long;
    using period     = std::micro;
    using duration   = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<HighResTimer>;
    // Mark it as steady
    static const bool is_steady = true;
    // Retrieves the current value of the high resolution performance counter
    static time_point now();
    // Returns the number of milliseconds since timer reset
    static uint time_ms();
};
