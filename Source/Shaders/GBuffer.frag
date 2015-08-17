#version 440

#define SQRT_2 1.4142135

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (early_fragment_tests) in;           // Force early Z-test (prior to FS)

smooth in vec3 w_pos;                       // Fragment position in world space
smooth in vec3 w_norm;                      // Fragment normal in world space

layout (location = 0) uniform int mat_id;   // Material index

// Vars OUT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (location = 0) out vec3 w_position;  // Position in world space
layout (location = 1) out vec2 w_enc_norm;  // Encoded normal in world space
layout (location = 2) out uint material_id; // Material index

// Implementation >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// Lambert's azimuthal equal-area projection
// https://en.wikipedia.org/wiki/Lambert_azimuthal_equal-area_projection
vec2 lambertAzimEAProj(in const vec3 n) {
    const float abs_z = abs(n.z);
    if (abs_z <= 0.999) {
        // Regular case
        const float f = SQRT_2 / sqrt(1.0 - n.z);
        return f * n.xy;
    } else {
        // Special case
        // (0, 0, 1) and (0, 0, -1) both map to (0, 0)
        const float sgn_1 = n.z / abs_z;
        return sgn_1 * vec2(2.0, -1.0);
    }
}

void main() {
    w_position  = w_pos;
    w_enc_norm  = lambertAzimEAProj(w_norm);
    material_id = mat_id;
}
