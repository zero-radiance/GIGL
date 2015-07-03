#include "Random.h"
#include <random>

std::mt19937 UnitRNG::m_gen;

void UnitRNG::init() {
    static std::random_device rd;
    m_gen = std::mt19937{rd()};
}

float UnitRNG::generate() {
    #ifdef BROKEN_STD_GENERATE_CANONICAL
        static const std::uniform_real_distribution<float> dis{0.0, 1.0};
        return dis(m_gen);
    #else
        return std::generate_canonical<float, std::numeric_limits<float>::digits>(gen);
    #endif
}
