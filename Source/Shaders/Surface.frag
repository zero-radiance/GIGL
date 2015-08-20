#version 440

#define INV_PI        0.318309873       // 1 / π
#define CAM_RES       1024               // Camera sensor resolution
#define HG_G          0.25              // Henyey-Greenstein scattering asymmetry parameter
#define R_M_INTERVALS 8                 // Number of ray marching intervals
#define CLAMP_DIST_SQ 75.0 * 75.0       // Radius squared used for clamping
#define MAX_MATERIALS 8                 // Max. number of materials
#define MAX_PPLS      1                 // Max. number of primary lights
#define MAX_VPLS      150               // Max. number of secondary lights
#define MAX_FRAMES    30                // Max. number of frames before convergence is achieved
#define SAFE          restrict coherent // Assume coherency within shader, enforce it between shaders

struct Material {
    vec3  k_d;                          // Diffuse coefficient
    vec3  k_s;                          // Specular coefficient
    float n_s;                          // Specular exponent
    vec3  k_e;                          // Emission [coefficient]
};

struct PrimaryPointLight {
    vec3 w_pos;                         // Position in world space
    vec3 intens;                        // Light intensity
};

struct VirtualPointLight {
    vec3  w_pos;                        // Position in world space             | Volume & Surface
    uint  type;                         // 1: VPL in volume, 2: VPL on surface | Volume & Surface
    vec3  intens;                       // Intensity (incident radiance)       | Volume & Surface
    float sca_k;                        // Scattering coefficient              | Volume only
    vec3  w_inc;                        // Incoming direction in world space   | Volume & Surface
    uint  path_id;                      // Path index                          | Volume & Surface
    vec3  w_norm;                       // Normal direction in world space     | Surface only
    vec3  k_d;                          // Diffuse coefficient                 | Surface only
    vec3  k_s;                          // Specular coefficient                | Surface only
    float n_s;                          // Specular exponent                   | Surface only
};

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (std140, binding = 0)
uniform Materials {
    Material materials[MAX_MATERIALS];
};

layout (std140, binding = 1)
uniform PPLs {
    PrimaryPointLight ppls[3 * MAX_PPLS];
};

layout (std140, binding = 2)
uniform VPLs {
    VirtualPointLight vpls[3 * MAX_VPLS];
};

// Omnidirectional shadow mapping
uniform int                    n_vpls;          // Number of active VPLs
uniform samplerCubeArrayShadow ppl_shadow_cube; // Cubemap array of shadowmaps of PPLs
uniform samplerCubeArrayShadow vpl_shadow_cube; // Cubemap array of shadowmaps of VPLs
uniform float                  inv_max_dist_sq; // Inverse max. [shadow] distance squared

// G-buffer
uniform sampler2D     w_positions;      // Per-fragment position(s) in world space
uniform sampler2D     enc_w_normals;    // Encoded per-fragment normal(s) in world space
uniform usampler2D    material_ids;     // Per-fragment material indices

// Fog
uniform sampler3D     vol_dens;         // Normalized volume density (3D texture)
uniform sampler3D     pi_dens;          // Preintegrated fog density values
uniform vec3          fog_bounds[2];    // Minimal and maximal bounding points of fog volume
uniform vec3          inv_fog_dims;     // Inverse of fog dimensions
uniform float         ext_k;            // Extinction coefficient per unit density
uniform float         sca_albedo;       // Probability of photon being scattered
uniform SAFE writeonly layout(rg32f) image2D fog_dist;  // Primary ray entry/exit distances

// Misc
uniform bool          gi_enabled;       // Flag indicating whether Global Illumination is enabled
uniform bool          clamp_rsq;        // Determines whether radius squared is clamped
uniform int           frame_id;         // Frame index, is set to zero on reset
uniform int           exposure;         // Exposure time
uniform vec3          cam_w_pos;        // Camera position in world space
uniform int           tri_buf_idx;      // Active buffer index within ring-triple-buffer
uniform SAFE layout(rgba32f) image2D accum_buffer;      // Accumulation buffer

// Vars OUT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (location = 0) out vec3 frag_col;

// Implementation >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// The inverse of Lambert's azimuthal equal-area projection
// https://en.wikipedia.org/wiki/Lambert_azimuthal_equal-area_projection
vec3 invLambertAzimEAProj(const vec2 n) {
    if (abs(n.x) != 2.0) {
        // Regular case
        const float d = dot(n, n);
        const float f = sqrt(1.0 - 0.25 * d);
        return vec3(f * n, -1.0 + 0.5 * d);
    } else {
        // Special case
        // Map (2, -1) to (0, 0, 1) and (-2, 1) to (0, 0, -1)
        return vec3(0.0, 0.0, n.x + n.y);
    }
}

// Returns fragment's position in world space
vec3 getWorldPos() {
    return texelFetch(w_positions, ivec2(gl_FragCoord.xy), 0).rgb;
}

// Returns fragment's normal in world space
vec3 getWorldNorm() {
    const vec2 enc_norm = texelFetch(enc_w_normals, ivec2(gl_FragCoord.xy), 0).rg;
    return invLambertAzimEAProj(enc_norm);
}

// Returns fragment's material in world space
Material getMaterial() {
    const uint mat_id = texelFetch(material_ids, ivec2(gl_FragCoord.xy), 0).r;
    return materials[mat_id];
}

// Saves parametric ray distances to a texture
void recordRayDist(in const float t_min, in const float t_max) {
    imageStore(fog_dist, ivec2(gl_FragCoord.xy), vec4(t_min, t_max, 0.0, 0.0));
}

// Returns the color value from the accumulation buffer
vec3 readFromAccumBuffer() {
    return imageLoad(accum_buffer, ivec2(gl_FragCoord.xy)).rgb;
}

// Writes the color value to the accumulation buffer
void writeToAccumBuffer(in const vec3 color) {
    imageStore(accum_buffer, ivec2(gl_FragCoord.xy), vec4(color, 1.0));
}

// Performs ray-BBox intersection
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

// Computes fog density at the specified world position
float calcFogDens(in const vec3 w_pos) {
    const vec3 r_pos = w_pos - fog_bounds[0];
    const vec3 n_pos = r_pos * inv_fog_dims;
    return texture(vol_dens, n_pos).r;
}

// Returns the extinction coefficient at the specified world space position
float sampleExtK(in const vec3 w_pos) {
    return ext_k * calcFogDens(w_pos);
}

// Performs ray marching
float rayMarch(in const vec3 ray_o, in const vec3 ray_d,
               in const float t_min, in const float t_max) {
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
    if (ext_k > 0.0) {
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

// Evaluates the Phong BRDF
vec3 phongBRDF(in const vec3 I, in const vec3 N, in const vec3 O,
               in const vec3 k_d, in const vec3 k_s, in const float n_s) {
    vec3 brdf_val = k_d;
    if (n_s > 0.0) {
        // Compute the specular (mirror-reflected) direction
        const vec3 S = reflect(-I, N);    
        if (dot(O, S) > 0.0) {
            brdf_val += k_s * pow(dot(O, S), n_s);
        }
    }
    return brdf_val;
}

// Evaluates the Henyey-Greenstein phase function
float evalPhaseHG(in const float cos_the) {
    const float base = 1.0 + HG_G * HG_G - 2.0 * HG_G * cos_the;
    return 0.25 * INV_PI * (1.0 - HG_G * HG_G) / (base * sqrt(base));
}

// Returns radiance emitted (scattered) by a VPL in the given direction
vec3 computeLe(in const VirtualPointLight vpl, in const vec3 O) {
    switch (vpl.type) {
        case 1: // VPL in volume
        {
            const float cos_the = dot(O, vpl.w_inc);
            // WT implicitly accounts for extinction
            // Therefore, use scattering albedo and not scattering coefficient
            return evalPhaseHG(cos_the) * sca_albedo * vpl.intens;
        }
        case 2: // VPL on surface
        {
            // Compute reflected radiance
            const float cos_the_out = max(0.0, dot(O,  vpl.w_norm));
            return phongBRDF(vpl.w_inc, vpl.w_norm, O, vpl.k_d, vpl.k_s, vpl.n_s) * vpl.intens * cos_the_out;
        }
    }
}

// Compute the contribution of primary point lights
vec3 calcPplContrib(in const int light_id, in const vec3 w_pos, in const vec3 N, in const vec3 O,
                    in const Material material) {
    const vec3  d = w_pos - ppls[light_id].w_pos;
    const float dist_sq = dot(d, d);
    // Check OSM visibility
    const float norm_dist_sq = dist_sq * inv_max_dist_sq;
    // dist < texture(x, y, z, i) ? 1.0 : 0.0
    const float visibility = texture(ppl_shadow_cube, vec4(d, light_id), norm_dist_sq);
    if (visibility > 0.0) {
        // Light is visible from the fragment
        const vec3  I       = normalize(-d);
        const float transm  = calcTransm(w_pos, I, dist_sq);
        const float falloff = 1.0 / dist_sq;
        const vec3  Li      = transm * ppls[light_id].intens * falloff;
        // Evaluate the rendering equation
        const float cos_the_inc = max(0.0, dot(I, N));
        return phongBRDF(I, N, O, material.k_d, material.k_s, material.n_s) * Li * cos_the_inc;
    } else {
        return vec3(0.0);
    }
}

// Compute the contribution of VPLs
vec3 calcVplContrib(in const int light_id, in const vec3 w_pos, in const vec3 N, in const vec3 O,
                    in const Material material) {
    const vec3  d = w_pos - vpls[light_id].w_pos;
    const float dist_sq = dot(d, d);
    // Check OSM visibility
    const float norm_dist_sq = dist_sq * inv_max_dist_sq;
    // dist < texture(x, y, z, i) ? 1.0 : 0.0
    const float visibility = texture(vpl_shadow_cube, vec4(d, light_id), norm_dist_sq);
    if (visibility > 0.0) {
        // Light is visible from the fragment
        const vec3  I       = normalize(-d);
        const float transm  = calcTransm(w_pos, I, dist_sq);
        const float falloff = 1.0 / max(dist_sq, CLAMP_DIST_SQ);
        const vec3  Li      = transm * computeLe(vpls[light_id], -I) * falloff;
        // Evaluate the rendering equation
        const float cos_the_inc = max(0.0, dot(I, N));
        return phongBRDF(I, N, O, material.k_d, material.k_s, material.n_s) * Li * cos_the_inc;
    } else {
        return vec3(0.0);
    }
}

void main() {
    frag_col = vec3(0.0);
    if (frame_id < MAX_FRAMES) {
        // Perform shading
        float transm_frag = 1.0;
        const vec3 w_pos  = getWorldPos();
        const vec3 ray_d  = normalize(w_pos - cam_w_pos);
        if (ext_k > 0.0) {
            // Compute the intersection with fog volume
            const vec3  ray_o  = cam_w_pos;
            const float t_frag = length(w_pos - cam_w_pos);
            float t_min, t_max;
            if (intersectBBox(fog_bounds, ray_o, ray_d, t_frag, t_min, t_max)) {
                t_min = max(t_min, 0.0);
                t_max = min(t_max, t_frag);
                // Compute transmittance from camera to fragment along primary ray
                const vec2 xy = gl_FragCoord.xy / CAM_RES;
                const float z = (t_frag - t_min) / (t_max - t_min);
                const float opt_depth = ext_k * texture(pi_dens, vec3(xy, z)).r;
                transm_frag = exp(-opt_depth);
            } else {
                t_min = t_max = 0.0;
            }
            // Record t_min and t_max values for reuse in the subsequent shader
            recordRayDist(t_min, t_max);
        }
        if (transm_frag > 0.001) {
            const vec3     w_norm   = getWorldNorm();
            const Material material = getMaterial();
            if (clamp_rsq) {
                // Gather contribution of primary lights
                for (int i = tri_buf_idx * MAX_PPLS, e = i + MAX_PPLS; i < e; ++i) {
                    frag_col += transm_frag * calcPplContrib(i, w_pos, w_norm, -ray_d, material);
                }
            }
            if (gi_enabled) {
                // Gather contribution of VPLs
                for (int i = tri_buf_idx * MAX_VPLS, e = i + n_vpls; i < e; ++i) {
                    frag_col += transm_frag * calcVplContrib(i, w_pos, w_norm, -ray_d, material);
                }
            }
        }
        if (frame_id > 0) {
            // Read the value from the previous frame
            const vec3 prev_color = readFromAccumBuffer();
            // Accumulate radiance
            frag_col += prev_color;
        }
        // Update the accumulation buffer
        writeToAccumBuffer(frag_col);
    } else {
        // Do nothing; accumulation buffer values are final
    }
}
