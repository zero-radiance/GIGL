#pragma once

#include <GLM\vec3.hpp>
#include "..\GL\GLShader.h"
#include "..\GL\GLVertArray.h"
#include "..\GL\GLTextureBuffer.h"
#include "..\GL\GLUniformManager.h"
#include "..\GL\GLTexture2D.h"
#include "..\VPL\OmniShadowMap.h"

class PPL;
class VPL;
class Scene;
class PerspectiveCamera;
template <class T> class LightArray;

/* Rendering parameters updated by InputHandler */
struct RenderSettings {
    RenderSettings() = default;
    RULE_OF_ZERO(RenderSettings);
    bool      gi_enabled;       // Indicates whether Global Illumination is enabled
    bool      clamp_r_sq;       // Indicates whether radius squared of VPLs is being clamped
    int       exposure;         // Exposure time; higher values increase brightness
    int       frame_num;        // Current frame number
    uint      curr_time_ms;     // Number of milliseconds since timer reset (curr. frame)
    int       max_num_vpls;     // Maximal number of active VPLs
    float     abs_k;            // Absorption coefficient per unit density
    float     sca_k;            // Scattering coefficient per unit density
    float     maj_ext_k;        // Majorant extinction coefficient of fog
    glm::vec3 ppl_w_pos;        // Primary point light's position in world coordinates
};

/* Implements OpenGL deferred renderer functionality */
class DeferredRenderer {
public:
    DeferredRenderer(const int res_x, const int res_y);
    RULE_OF_FIVE_NO_COPY(DeferredRenderer);
    // Returns the shader program responsible for generating a G-buffer
    const GLSLProgram& getGBufProgram() const;
    // Returns the shader program responsible for shading 
    const GLSLProgram& getShadingProgram() const;
    // Updates the primary lights and the VPLs (using the settings)
    void updateLights(const Scene& scene, const glm::vec3& target,
                      LightArray<PPL>& ppls, LightArray<VPL>& vpls);
    // Generates shadow maps
    void generateShadowMaps(const Scene& scene, const glm::mat4& model_mat,
                            const LightArray<PPL>& ppls,
                            const LightArray<VPL>& vpls) const;
    // Generates a G-buffer with positions, normals and material ids
    void generateGBuffer(const Scene& scene) const;
    // Performs shading
    void shade(const int tri_buf_idx) const;
    // Public data members
    RenderSettings settings;                // Rendering parameters updated by InputHandler
private:
    GLsizei             m_res_x, m_res_y;   // Viewport width and height
    GLSLProgram         m_sp_osm;           // GLSL program which generates omnidir. shadow maps
    GLSLProgram         m_sp_gbuf;          // GLSL program which fills a G-buffer
    GLSLProgram         m_sp_shade;         // GLSL program which performs shading
    GLTextureBuffer     m_hal_tbo;          // Halton sequence texture buffer object
    GLUniformManager<9> m_uniform_mngr;     // OpenGL uniform manager
    OmniShadowMap       m_ppl_OSM;          // Omnidirectional shadow map for primary lights
    OmniShadowMap       m_vpl_OSM;          // Omnidirectional shadow map for VPLs
    GLuint              m_defer_fbo_handle; // Deferred framebuffer handle
    GLuint              m_depth_rbo_handle; // Depth renderbuffer handle
    GLVertArray         m_ss_quad_va;       // Vertex array with a screen space quad
    GLTex2D_3x32F       m_tex_w_pos;        // World position texture
    GLTex2D_2x32F       m_tex_w_norm;       // Normal vector texture
    GLTex2D_1x8UI       m_tex_mat_id;       // Material id texture
};
