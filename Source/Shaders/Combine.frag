#version 440

#define CAM_RES    1024                  // Camera sensor resolution
#define MAX_FRAMES 30                   // Max. number of frames before convergence is achieved
#define SAFE       restrict coherent    // Assume coherency within shader, enforce it between shaders

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uniform int        exposure;            // Exposure time
uniform int        frame_id;            // Frame index, is set to zero on reset
uniform sampler2D  vol_comp;            // Subsampled volume contribution (radiance)
uniform sampler2D  depth_buf;           // Depth buffer
uniform float      ext_k;               // Extinction coefficient per unit density
uniform SAFE layout(rgba32f) image2D accum_buffer;  // Accumulation buffer

// Vars OUT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (location = 0) out vec3 frag_col;

// Implementation >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// Performs bilateral upsampling of color texture using depth texture around the given position
vec3 bilateralUpsampling(in const sampler2D color_tex, in const sampler2D depth_tex,
                         in const vec2 pos) {
    const ivec2 center = ivec2(pos);
    const float center_depth = texelFetch(depth_tex, center, 0).r;
    // Initialize the accumulated color and depth to 0
    float accum_weight = 0.0;
    vec3  accum_color  = vec3(0.0);
    // Accumulate over the 3x3 pixels neighbourhood
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            const ivec2 pos    = center + ivec2(x, y);
            const float depth  = texelFetch(depth_tex, pos, 0).r;
            const vec2  n_pos  = vec2(pos) / float(CAM_RES);
            const vec3  color  = texture(color_tex, n_pos).rgb;
            const float weight = 1.0 / (1000.0 * abs(depth - center_depth) + 1.0);
            accum_weight += weight;
            accum_color  += weight * color;
        }
    }
    // Perform normalization
    return accum_color / accum_weight;    
}

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
        // Perform bilateral upsampling
        const vec3 vol_col = bilateralUpsampling(vol_comp, depth_buf, gl_FragCoord.xy);
        // Add the upsampled volume contribution
        frag_col += vol_col;
        // Store the combined value
        writeToAccumBuffer(frag_col);
    } else {
        // Do nothing; accumulation buffer values are final
    }
    // Multi-frame accumulation: normalize
    frag_col /= (1 + min(frame_id, 30));
    // Perform tone mapping
    frag_col = vec3(1.0) - exp(-exposure * frag_col);
}
