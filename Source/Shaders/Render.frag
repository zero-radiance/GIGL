#version 440

#define PI            3.14159274    // π
#define INV_PI        0.318309873   // 1 / π
#define CAM_RES       800           // Camera sensor resolution
#define GAMMA         vec3(2.2)     // Gamma value for gamma correction
#define HG_G          0.25          // Henyey-Greenstein scattering asymmetry parameter
#define N_PPLS        1             // Number of primary lights
#define N_VPLS        150           // Number of secondary lights
#define OFFSET        0.001         // Shadow mapping offset
#define R_M_INTERVALS 8             // Number of ray marching intervals
#define CLAMP_DIST_SQ 75.0 * 75.0   // Radius squared used for clamping
#define MAX_FRAMES    30            // Maximal number of frames before convergence is achieved
#define MAX_SAMPLES   24            // Maximal number of samples per pixel

struct PriLight {
    vec3 w_pos;                     // Position in world space
    vec3 intens;                    // Light intensity
};

struct SecLight {
    vec3  w_pos;                    // Position in world space                | Volume and Surface
    uint  type;                     // 1 = VPL in volume, 2 = VPL on surface  | Volume and Surface
    vec3  intens;                   // Intensity (incident radiance)          | Volume and Surface
    float sca_k;                    // Scattering coefficient                 | Volume only
    vec3  w_inc;                    // Incoming direction in world space      | Volume and Surface
    uint  path_id;                  // Path index                             | Volume and Surface
    vec3  w_norm;                   // Normal direction in world space        | Surface only
    vec3  k_d;                      // Diffuse coefficient                    | Surface only
    vec3  k_s;                      // Specular coefficient                   | Surface only
    float n_s;                      // Specular exponent                      | Surface only
};

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (early_fragment_tests) in;   // Force early Z-test (prior to fragment shader)

smooth in vec3 w_pos;               // Fragment position in world space
smooth in vec3 w_norm;              // Fragment normal in world space

layout (binding = 0)
uniform MaterialInfo {
    vec3  k_d;                      // Diffuse coefficient
    vec3  k_s;                      // Specular coefficient
    vec3  k_e;                      // Emission
    float n_s;                      // Specular exponent
} material;

layout (std140, binding = 1)
uniform PriLightInfo {
    PriLight ppls[3 * N_PPLS];
};

layout (std140, binding = 2)
uniform SecLightInfo {
    SecLight vpls[3 * N_VPLS];
};

// Omnidirectional shadow mapping
uniform int                    n_vpls;              // Number of active VPLs
uniform samplerCubeArrayShadow ppl_shadow_cube;     // Cubemap array of shadowmaps of PPLs
uniform samplerCubeArrayShadow vpl_shadow_cube;     // Cubemap array of shadowmaps of VPLs
uniform float                  inv_max_dist_sq;     // Inverse max. [shadow] distance squared

// Fog
uniform sampler3D     vol_dens;         // Normalized volume density (3D texture)
uniform sampler3D     pi_dens;          // Preintegrated fog density values
uniform vec3          fog_bounds[2];    // Minimal and maximal bounding points of fog volume
uniform vec3          inv_fog_dims;     // Inverse of fog dimensions
uniform float         sca_k;            // Scattering coefficient per unit density
uniform float         ext_k;            // Extinction coefficient per unit density
uniform float         sca_albedo;       // Probability of photon being scattered
uniform bool          clamp_rsq;        // Determines whether radius squared is clamped

// Misc
uniform bool          gi_enabled;       // Flag indicating whether Global Illumination is enabled
uniform int           frame_id;         // Frame index, is set to zero on reset
uniform int           exposure;         // Exposure time for basic brightness control
uniform sampler2D     accum_buffer;     // Accumulation buffer (texture)
uniform vec3          cam_w_pos;        // Camera position in world space
uniform int           tri_buf_idx;      // Active buffer index within ring-triple-buffer
uniform samplerBuffer halton_seq;       // Halton sequence of size MAX_FRAMES * MAX_SAMPLES

// Vars OUT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (location = 0) out vec4 frag_col;

// Implementation >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

bool intersectBBox(in const vec3 bound_pts[2], in const vec3 ray_o, in const vec3 ray_d,
                   in const float max_dist, out float t_min, out float t_max) {
    const vec3 inv_ray_d = 1.0 / ray_d;

    float t0 = (bound_pts[0][0] - ray_o[0]) * inv_ray_d[0];
    float t1 = (bound_pts[1][0] - ray_o[0]) * inv_ray_d[0];
 
    t_min = min(t0, t1);
    t_max = max(t0, t1);
 
    for (int i = 1; i < 3; ++i) {
        t0 = (bound_pts[0][i] - ray_o[i]) * inv_ray_d[i];
        t1 = (bound_pts[1][i] - ray_o[i]) * inv_ray_d[i];
        t_min = max(t_min, min(t0, t1));
        t_max = min(t_max, max(t0, t1));
    }
 
    return t_max > max(t_min, 0.0) && t_min < max_dist;
}

float calcFogDens(in const vec3 w_pos) {
    const vec3 r_pos = w_pos - fog_bounds[0];
    const vec3 n_pos = r_pos * inv_fog_dims;
    return texture(vol_dens, n_pos).r;
}

float sampleScaK(in const vec3 w_pos) {
    return sca_k * calcFogDens(w_pos);
}

float sampleExtK(in const vec3 w_pos) {
    return ext_k * calcFogDens(w_pos);
}

// Performs ray marching
float rayMarch( in const vec3 ray_o, in const vec3 ray_d, in const float t_min, in const float t_max ) {
    const float dt    = (t_max - t_min) / R_M_INTERVALS;
    float prev_ext_k  = sampleExtK(ray_o + t_min * ray_d);
    float total_ext_k = 0.0;
    for (int i = 1; i <= R_M_INTERVALS; ++i) {
        // Distance to the end of the interval
        const float t = t_min + i * dt;
        const float curr_ext_k = sampleExtK(ray_o + t * ray_d);
        // Use trapezoidal rule for integration
        total_ext_k += 0.5 * (prev_ext_k + curr_ext_k);
        prev_ext_k = curr_ext_k;
    }
    const float opt_depth = total_ext_k * dt;
    // Apply Beer's law
    return exp(-opt_depth);
}

// Calculates transmittance using ray marching
float calcTransm(in const vec3 samp_pt, in const vec3 dir, in const float dist_sq) {
    float transm = 1.0;
    if (sca_k > 0.0) {
        const float max_dist = sqrt(dist_sq);
        float t_min, t_max;
        if (intersectBBox(fog_bounds, samp_pt, dir, max_dist, t_min, t_max)) {
            t_min  = max(t_min, 0.0);
            t_max  = min(t_max, max_dist);
            transm = rayMarch(samp_pt, dir, t_min, t_max);
        }
    }
    return transm;
}

// Evaluates Phong BRDF
vec3 phongBRDF(in const vec3 I, in const vec3 N, in const vec3 R,
                in const vec3 k_d, in const vec3 k_s, in const float n_s ) {
    // Compute specular (mirror-reflected) direction
    const vec3 S = reflect(-I, N);
    if (n_s > 0.0 && dot(R, S) > 0.0) {
        return k_d + k_s * pow(dot(R, S), n_s);
    } else {
        return k_d;
    }
}

// Evaluates Henyey-Greenstein phase function
float evalPhaseHG(in const float cos_the) {
    const float base = 1.0 + HG_G * HG_G - 2.0 * HG_G * cos_the;
    return 0.25 * INV_PI * (1.0 - HG_G * HG_G) / (base * sqrt(base));
}

// Returns radiance emitted in given direction
vec3 calcLe(in const SecLight vpl, in const vec3 LtoP) {
    switch (vpl.type) {
        case 1: // VPL in volume
        {
            const float cos_the = dot(LtoP, vpl.w_inc);
            // WT implicitly accounts for extinction
            // Therefore, use scattering albedo and not scattering coefficient
            return evalPhaseHG(cos_the) * sca_albedo * vpl.intens;
        }
        case 2: // VPL on surface
        {
            // Compute reflected radiance
            const float cos_the_out = max(0.0, dot(LtoP,  vpl.w_norm));
            return phongBRDF(vpl.w_inc, vpl.w_norm, LtoP, vpl.k_d, vpl.k_s, vpl.n_s) * vpl.intens * cos_the_out;
        }
    }
}

// Evaluates rendering equation
vec3 evalRE(in const vec3 I, in const vec3 O, in const vec3 Li, in const vec3 samp_pt, in const bool is_surf_pt) {
    if (is_surf_pt) {
        const float cos_the_inc = max(0.0, dot(I, w_norm));
        return phongBRDF(I, w_norm, O, material.k_d, material.k_s, material.n_s) * Li * cos_the_inc;
    } else {
        // Incoming direction points TOWARDS the light, so we have to reverse it
        const float cos_the = -dot(I, O);
        return evalPhaseHG(cos_the) * sampleScaK(samp_pt) * Li;
    }
}

// Compute contribution of primary lights
vec3 calcPriLSContrib(in const int light_id, in const vec3 samp_pt, in const vec3 pri_ray_d, in const bool is_surf_pt) {
    vec3 LtoP = samp_pt - ppls[light_id].w_pos;
    const float dist_sq = dot(LtoP, LtoP);
    LtoP = normalize(LtoP);
    // Check OSM visibility
    const float norm_dist_sq = dist_sq * inv_max_dist_sq + OFFSET;
    // dist < texture(x, y, z, i) ? 1.0 : 0.0
    const float visibility = texture(ppl_shadow_cube, vec4(LtoP, light_id), norm_dist_sq);
    if (0.0 == visibility) return vec3(0.0);
    // LS is visible from sample point
    const float transm  = calcTransm(samp_pt, -LtoP, dist_sq);
    const float falloff = 1.0 / max(dist_sq, CLAMP_DIST_SQ);
    const vec3  Li      = transm * ppls[light_id].intens * falloff;
    // Evaluate rendering equation at sample point
    return evalRE(-LtoP, -pri_ray_d, Li, samp_pt, is_surf_pt);
}

// Compute contribution of secondary lights
vec3 calcSecLSContrib(in const int light_id, in const vec3 samp_pt, in const vec3 pri_ray_d, in const bool is_surf_pt) {
    vec3 LtoP = samp_pt - vpls[light_id].w_pos;
    const float dist_sq = dot(LtoP, LtoP);
    LtoP = normalize(LtoP);
    // Check OSM visibility
    const float norm_dist_sq = dist_sq * inv_max_dist_sq + OFFSET;
    // dist < texture(x, y, z, i) ? 1.0 : 0.0
    const float visibility = texture(vpl_shadow_cube, vec4(LtoP, light_id), norm_dist_sq);
    if (0.0 == visibility) return vec3(0.0);
    // LS is visible from sample point
    const float transm  = calcTransm(samp_pt, -LtoP, dist_sq);
    const float falloff = 1.0 / (clamp_rsq ? max(dist_sq, CLAMP_DIST_SQ) : dist_sq);
    const vec3  Li      = transm * calcLe(vpls[light_id], LtoP) * falloff;
    // Evaluate rendering equation at sample point
    return evalRE(-LtoP, -pri_ray_d, Li, samp_pt, is_surf_pt);
}

// Returns color value from accumulation buffer
vec3 queryAccumBuffer() {
    return texelFetch(accum_buffer, ivec2(gl_FragCoord.xy), 0).rgb;
}

void main() {
    if (frame_id < MAX_FRAMES) {
        // Perform shading
        vec3 color = vec3(0.0);
        if (material.k_e != vec3(0.0)) {
            // Return emission value
            color = material.k_e;
        } else {
            float transm_frag = 1.0;
            const vec3 ray_d = normalize(w_pos - cam_w_pos);
            if (sca_k > 0.0) {
                const vec2 xy = gl_FragCoord.xy / CAM_RES;
                // Compute intersection with fog volume
                const vec3  ray_o  = cam_w_pos;
                const float t_frag = length(w_pos - cam_w_pos);
                float t_min, t_max;
                if (intersectBBox(fog_bounds, ray_o, ray_d, t_frag, t_min, t_max)) {
                    t_min = max(t_min, 0.0);
                    t_max = min(t_max, t_frag);
                    /* Compute volume contribution */
                    const int n_samples = gi_enabled ? MAX_SAMPLES / 4 : MAX_SAMPLES;
                    for (int s = 0; s < n_samples; ++s) {
                        // Compute sample position
                        const float z = texelFetch(halton_seq, s + n_samples * frame_id).r;
                        const float t = t_min + z * (t_max - t_min);
                        const vec3  s_pos = ray_o + t * ray_d;
                        // Compute transmittance on camera-to-sample interval
                        const float opt_depth = ext_k * texture(pi_dens, vec3(xy, z)).r;
                        const float transm = exp(-opt_depth);
                        if (transm > 0.001) {
                            if (clamp_rsq) {
                                // Gather contribution of primary lights
                                for (int i = tri_buf_idx * N_PPLS, e = i + N_PPLS; i < e; ++i) {
                                    color += transm * calcPriLSContrib(i, s_pos, ray_d, false);
                                }
                            }
                            if (gi_enabled) {
                                // Gather contribution of secondary lights
                                for (int i = tri_buf_idx * N_VPLS, e = i + n_vpls; i < e; ++i) {
                                    color += transm * calcSecLSContrib(i, s_pos, ray_d, false);
                                }
                            }
                        }
                    }
                    // Normalize
                    const float inv_p = t_max - t_min;
                    color *= inv_p / n_samples;
                    // Compute transmittance from camera to fragment along primary ray
                    const float z = (t_frag - t_min) / (t_max - t_min);
                    const float opt_depth = ext_k * texture(pi_dens, vec3(xy, z)).r;
                    transm_frag = exp(-opt_depth);
                }
            }
            /* Compute surface contribution */
            if (transm_frag > 0.001) {
                if (clamp_rsq) {
                    // Gather contribution of primary lights
                    for (int i = tri_buf_idx * N_PPLS, e = i + N_PPLS; i < e; ++i) {
                        color += transm_frag * calcPriLSContrib(i, w_pos, ray_d, true);
                    }
                }
                if (gi_enabled) {
                    // Gather contribution of secondary lights
                    for (int i = tri_buf_idx * N_VPLS, e = i + n_vpls; i < e; ++i) {
                        color += transm_frag * calcSecLSContrib(i, w_pos, ray_d, true);
                    }
                }
            }
        }
        // Perform tone mapping
        color = vec3(1.0) - exp(-exposure * color);
        if (frame_id > 0) {
            // Query the old value; undo gamma correction
            const vec3 prev_color = pow(queryAccumBuffer(), GAMMA);
            // Blend values
            color = (color + frame_id * prev_color) / (frame_id + 1);
        }
        // Perform gamma correction
        color = pow(color, 1.0 / GAMMA);
        // Write the result
        frag_col = vec4(color, 1.0);
    } else {
        // Return value from previous frame
        frag_col = vec4(queryAccumBuffer(), 1.0);
    }
}
