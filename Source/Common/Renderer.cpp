#include "Renderer.h"
#include "Constants.h"
#include "Utility.hpp"
#include "Halton.hpp"
#include "..\GL\GLShader.hpp"
#include "..\GL\GLUniformManager.hpp"
#include "..\RT\PhotonTracer.h"
#include "..\VPL\PointLight.hpp"
#include "..\VPL\OmniShadowMap.hpp"

using glm::vec3;
using glm::mat4;
using glm::normalize;

DeferredRenderer::DeferredRenderer():
                  m_ppl_OSM{PRI_SM_RES, 1,          MAX_DIST, TEX_U_PPL_SM},
                  m_vpl_OSM{SEC_SM_RES, MAX_N_VPLS, MAX_DIST, TEX_U_VPL_SM},
                  m_hal_tbo{30 * 24 * sizeof(GLfloat), TEX_U_HALTON, gl::R32F} {
    // Generate a Halton sequence for 30 frames with (up to) 24 samples per frame
    static const GLuint seq_sz{30 * 24};
    GLfloat hal_seq[seq_sz];
    HaltonSG::generate<2>(hal_seq);
    m_hal_tbo.bufferData(sizeof(hal_seq), hal_seq);
    // Load shaders which fill the G-buffer
    // ... TODO
    // Load shaders which perform shading
    m_sp_shade.loadShader("Source\\Shaders\\Render.vert");
    m_sp_shade.loadShader("Source\\Shaders\\Render.frag");
    m_sp_shade.link();
    m_sp_shade.use();
    // Manage the following uniforms automatically
    m_uniform_mngr.setManagedUniforms(m_sp_shade, {"gi_enabled", "clamp_rsq", "exposure",
                                                   "frame_id", "n_vpls", "sca_k", "ext_k",
                                                   "sca_albedo", "tri_buf_idx"});
}

const GLSLProgram& DeferredRenderer::getShaderProgram() const {
    return m_sp_shade;
}

void DeferredRenderer::updateLights(const Scene& scene, const vec3& target,
                                    LightArray<PPL>& ppls, LightArray<VPL>& vpls) {
    // Update the primary light
    ppls.clear();
    ppls.addLight(PPL{settings.ppl_w_pos, PRIM_PL_INTENS});
    const auto& prim_pl = ppls[0];
    // Update VPLs
    if (settings.gi_enabled) {
        const vec3 shoot_dir{normalize(target - prim_pl.wPos())};
        rt::PhotonTracer::trace(scene, prim_pl, shoot_dir, settings.max_num_vpls, vpls);
        // Disable GI on VPL tracing failure
        settings.gi_enabled = settings.gi_enabled && !vpls.isEmpty();
    }
}

void DeferredRenderer::generateShadowMaps(const Scene& scene, const mat4& model_mat,
                                          const LightArray<PPL>& ppls,
                                          const LightArray<VPL>& vpls) const {
    gl::CullFace(gl::FRONT);
    m_ppl_OSM.generate(scene, ppls, model_mat);
    if (settings.gi_enabled) m_vpl_OSM.generate(scene, vpls, model_mat);
}

void DeferredRenderer::shade(const Scene& scene, const int tri_buf_idx) const {
    m_sp_shade.use();
    // Set dynamic uniforms
    m_uniform_mngr.setUniformValues(settings.gi_enabled, settings.clamp_r_sq,
                                    settings.exposure, settings.frame_num,
                                    settings.gi_enabled ? settings.max_num_vpls : 0,
                                    settings.sca_k, settings.abs_k + settings.sca_k,
                                    settings.sca_k / (settings.abs_k + settings.sca_k),
                                    tri_buf_idx);
    // Render
    gl::CullFace(gl::BACK);
    scene.render();
}
