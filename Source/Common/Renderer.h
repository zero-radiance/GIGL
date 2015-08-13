#pragma once

#include <GLM\vec3.hpp>
#include "..\GL\GLShader.h"
#include "..\GL\GLTextureBuffer.h"
#include "..\GL\GLUniformManager.h"
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
    bool      gi_enabled;           // Indicates whether Global Illumination is enabled
    bool      clamp_r_sq;           // Indicates whether radius squared of VPLs is being clamped
    int       exposure;             // Exposure time; higher values increase brightness
    int       frame_num;            // Current frame number
    uint      curr_time_ms;         // Number of milliseconds since timer reset (curr. frame)
    int       max_num_vpls;         // Maximal number of active VPLs
    float     abs_k;                // Absorption coefficient per unit density
    float     sca_k;                // Scattering coefficient per unit density
    float     maj_ext_k;            // Majorant extinction coefficient of fog
    glm::vec3 ppl_w_pos;            // Primary point light's position in world coordinates
};

/* Implements OpenGL deferred renderer functionality */
class DeferredRenderer {
public:
    DeferredRenderer();
    RULE_OF_ZERO_NO_COPY(DeferredRenderer);
    // Updates the primary lights and the VPLs (using the settings)
    void updateLights(const Scene& scene, const glm::vec3& target,
                      LightArray<PPL>& ppls, LightArray<VPL>& vpls);
    // Generates shadow maps
    void generateShadowMaps(const Scene& scene, const glm::mat4& model_mat,
                            const LightArray<PPL>& ppls,
                            const LightArray<VPL>& vpls) const;
    // TODO
    void renderGeometry() const;
    // Performs shading
    void shade(const Scene& scene, const int tri_buf_idx) const;
    // Returns the (primary) shader program responsible for shading 
    const GLSLProgram& getShaderProgram() const;
    // Public data members
    RenderSettings settings;            // Rendering parameters updated by InputHandler
private:
    //GLSLProgram         m_sp_geom;    // GLSL program which fills a G-buffer
    GLSLProgram         m_sp_osm;       // GLSL program which generates omnidir. shadow maps
    GLSLProgram         m_sp_shade;     // GLSL program which performs shading
    GLUniformManager<9> m_uniform_mngr; // OpenGL uniform manager
    OmniShadowMap       m_ppl_OSM;      // Omnidirectional shadow map for primary lights
    OmniShadowMap       m_vpl_OSM;      // Omnidirectional shadow map for VPLs
    GLTextureBuffer     m_hal_tbo;      // Halton sequence texture buffer object
};
