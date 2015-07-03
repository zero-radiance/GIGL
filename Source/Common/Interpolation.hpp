#pragma once

template <typename T>
static inline T lerp1D(const T& p0, const T& p1, const float t) {
    return (1.0f - t) * p0 + t * p1;
}

template <typename T>
static inline T lerp2D(const T& p00, const T& p10, const T& p01, const T& p11,
                       const float tx, const float ty) {
    const T i0 = lerp1D(p00, p10, tx);
    const T i1 = lerp1D(p01, p11, tx);
    return lerp1D(i0, i1, ty);
}

template <typename T>
static inline T lerp3D(const T& p000, const T& p100, const T& p010, const T& p110,
                       const T& p001, const T& p101, const T& p011, const T& p111,
                       const float tx, const float ty, const float tz) {
    const T i0 = lerp2D(p000, p100, p010, p110, tx, ty);
    const T i1 = lerp2D(p001, p101, p011, p111, tx, ty);
    return lerp1D(i0, i1, tz);
}
