#include "Timer.h"
#include <Windows.h>

#ifdef min
    #undef min
#endif

#ifdef max
    #undef max
#endif

HighResTimer::time_point HighResTimer::now() {
    LARGE_INTEGER n_ticks, ticks_per_second;
    QueryPerformanceCounter(&n_ticks);
    QueryPerformanceFrequency(&ticks_per_second);
    // Avoid precision loss
    long long tmp = n_ticks.QuadPart * static_cast<rep>(period::den);
    // Return the number of microseconds
    return time_point(duration(tmp / ticks_per_second.QuadPart));
}

uint HighResTimer::time_ms() {
    const auto time_now = now().time_since_epoch();
    const auto ms_count = std::chrono::duration_cast<std::chrono::milliseconds>(time_now).count();
    return static_cast<uint>(ms_count);
}
