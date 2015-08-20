#version 440

#define CAM_RES    1024                  // Camera sensor resolution
#define MAX_FRAMES 30                   // Max. number of frames before convergence is achieved
#define SAFE       restrict coherent    // Assume coherency within shader, enforce it between shaders

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uniform int        exposure;            // Exposure time
uniform int        frame_id;            // Frame index, is set to zero on reset
uniform sampler2D  vol_comp;            // Subsampled volume contribution (radiance)
uniform float      ext_k;               // Extinction coefficient per unit density
uniform SAFE layout(rgba32f) image2D accum_buffer;  // Accumulation buffer

// Vars OUT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (location = 0) out vec3 frag_col;

// Implementation >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// Returns the color value from the accumulation buffer
vec3 readFromAccumBuffer() {
    return imageLoad(accum_buffer, ivec2(gl_FragCoord.xy)).rgb;
}

// Writes the color value to the accumulation buffer
void writeToAccumBuffer(in const vec3 color) {
    imageStore(accum_buffer, ivec2(gl_FragCoord.xy), vec4(color, 1.0));
}

void main() {
    // Read surface contribution
    frag_col = readFromAccumBuffer();
    if (frame_id < MAX_FRAMES && ext_k > 0.0) {
        // Add subsampled volume contribution
        frag_col += texture(vol_comp, gl_FragCoord.xy / CAM_RES).rgb;
        // Store the combined value
        writeToAccumBuffer(frag_col);
    } else {
        // Do nothing; accumulation buffer values are final
    }
    // Perform tone mapping
    frag_col = vec3(1.0) - exp(-exposure * frag_col);
}
