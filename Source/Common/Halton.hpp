#pragma once

/* Halton Sequence Generator */
namespace HaltonSG {
    // Generates Halton sequence with base B of size N
    template <int B, int N>
    void generate(float (&seq)[N]) {
        for (int i = 0; i < N; ++i) {
            seq[i] = 0.0f;
            int den{B}, t{i + 1};
            while (t > 0) {
                seq[i] += 1.0f / den * (t % B);
                t   /= B;
                den *= B;
            }
        }
    }
}
