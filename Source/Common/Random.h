#pragma once

#include "Definitions.h"

namespace std {
    template <class _Ty, size_t _Wx, size_t _Nx, size_t _Mx,
              size_t _Rx, _Ty _Px, size_t _Ux, _Ty _Dx,
              size_t _Sx, _Ty _Bx, size_t _Tx, _Ty _Cx,
              size_t _Lx, _Ty _Fx> class mersenne_twister_engine;

    using mt19937 = mersenne_twister_engine<uint, 32, 624, 397, 31, 0x9908b0df, 11, 0xffffffff, 7,
                                            0x9d2c5680, 15, 0xefc60000, 18, 1812433253>;
}

/* Random Number Generator on unit interval: [0, 1) */
class UnitRNG {
public:
    UnitRNG() = delete;
    RULE_OF_ZERO(UnitRNG);
    // Performs initialization
    static void init();
    // Generates a random single-precision float on [0, 1)
    static float generate();
private:
    static std::mt19937 m_gen;    // Mersenne Twister pseudorandom number generator
};
