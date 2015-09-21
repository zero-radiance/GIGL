#version 440

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

smooth in vec3 w_pos;                                   // Fragment position in world coords

layout (location = 8) uniform vec3  light_w_pos;        // Light position in world coords
layout (location = 9) uniform float inv_max_dist_sq;    // Inverse max distance squared

// Implementation >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void main() {
    // Compute vector from light to fragment
    const vec3  light_to_frag = w_pos - light_w_pos;
    // Compute distance squared
    const float dist_sq	= dot(light_to_frag, light_to_frag);
    // Normalize it s.t. it lies on [0, 1]
    const float norm_dist_sq = dist_sq * inv_max_dist_sq;
    gl_FragDepth = norm_dist_sq;
}
