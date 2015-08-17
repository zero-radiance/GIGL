#include "Renderer.h"
#include "Constants.h"
#include "Utility.hpp"
#include "Halton.hpp"
#include "..\GL\GLShader.hpp"
#include "..\GL\GLUniformManager.hpp"
#include "..\GL\GLTexture2D.hpp"
#include "..\RT\PhotonTracer.h"
#include "..\VPL\PointLight.hpp"
#include "..\VPL\OmniShadowMap.hpp"

using glm::vec3;
using glm::mat4;
using glm::normalize;

CONSTEXPR GLuint  ss_quad_va_components  = 1;    // Position
CONSTEXPR GLsizei ss_quad_va_comp_cnts[] = {3};  // vec3

DeferredRenderer::DeferredRenderer(const int res_x, const int res_y):
                  m_res_x{res_x}, m_res_y{res_y},
                  m_hal_tbo{30 * 24 * sizeof(GLfloat), TEX_U_HALTON, gl::R32F},
                  m_ppl_OSM{PRI_SM_RES, 1,          MAX_DIST, TEX_U_PPL_SM},
                  m_vpl_OSM{SEC_SM_RES, MAX_N_VPLS, MAX_DIST, TEX_U_VPL_SM},
                  m_ss_quad_va{ss_quad_va_components, ss_quad_va_comp_cnts},
                  m_tex_accum{TEX_U_ACCUM, res_x, res_y, false, false},
                  m_tex_w_pos{ TEX_U_W_POS,  res_x, res_y, false, false},
                  m_tex_w_norm{TEX_U_W_NORM, res_x, res_y, false, false},
                  m_tex_mat_id{TEX_U_MAT_ID, res_x, res_y, false, false} {
    // Generate a Halton sequence for 30 frames with (up to) 24 samples per frame
    static const GLuint seq_sz{30 * 24};
    GLfloat hal_seq[seq_sz];
    HaltonSG::generate<2>(hal_seq);
    m_hal_tbo.bufferData(sizeof(hal_seq), hal_seq);
    // Load shadow map generating program
    m_sp_osm.loadShader("Source\\Shaders\\Shadow.vert");
    m_sp_osm.loadShader("Source\\Shaders\\Shadow.geom");
    m_sp_osm.loadShader("Source\\Shaders\\Shadow.frag");
    m_sp_osm.link();
    // Load shaders which fill the G-buffer
    m_sp_gbuf.loadShader("Source\\Shaders\\GBuffer.vert");
    m_sp_gbuf.loadShader("Source\\Shaders\\GBuffer.frag");
    m_sp_gbuf.link();
    // Load shaders which perform shading
    m_sp_shade.loadShader("Source\\Shaders\\Shade.vert");
    m_sp_shade.loadShader("Source\\Shaders\\Shade.frag");
    m_sp_shade.link();
    m_sp_shade.use();
    // Manage the following uniforms automatically
    m_uniform_mngr.setManagedUniforms(m_sp_shade, {"gi_enabled", "clamp_rsq", "exposure",
                                                   "frame_id", "n_vpls", "sca_k", "ext_k",
                                                   "sca_albedo", "tri_buf_idx"});
    // Create a screen space quad
    CONSTEXPR float ss_quad_pos[] = {-1.0f, -1.0f, 0.0f,
                                      1.0f, -1.0f, 0.0f,
                                     -1.0f,  1.0f, 0.0f,
                                      1.0f,  1.0f, 0.0f};
    m_ss_quad_va.loadData(0, 12, ss_quad_pos);
    m_ss_quad_va.buffer();
    // Bind the accumulation texture to the image unit
    gl::BindImageTexture(IMG_U_ACCUM, m_tex_accum.id(), 0, false, 0, gl::READ_WRITE, gl::RGBA32F);
    // Generate and bind the FBO
    gl::GenFramebuffers(1, &m_defer_fbo_handle);
    gl::BindFramebuffer(gl::FRAMEBUFFER, m_defer_fbo_handle);
    // Generate and bind the depth buffer
    gl::GenRenderbuffers(1, &m_depth_rbo_handle);
    gl::BindRenderbuffer(gl::RENDERBUFFER, m_depth_rbo_handle);
    gl::RenderbufferStorage(gl::RENDERBUFFER, gl::DEPTH_COMPONENT, res_x, res_y);
    // Attach textures to the framebuffer
    gl::FramebufferRenderbuffer(gl::FRAMEBUFFER, gl::DEPTH_ATTACHMENT,
                                gl::RENDERBUFFER, m_depth_rbo_handle);
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D,
                             m_tex_w_pos.id(), 0);
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT1, gl::TEXTURE_2D,
                             m_tex_w_norm.id(), 0);
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT2, gl::TEXTURE_2D,
                             m_tex_mat_id.id(), 0);
    // Specify the buffers to draw to
    GLenum draw_buffers[] = {gl::COLOR_ATTACHMENT0, gl::COLOR_ATTACHMENT1, gl::COLOR_ATTACHMENT2};
    gl::DrawBuffers(3, draw_buffers);
    // Verify framebuffer
    const GLenum result{gl::CheckFramebufferStatus(gl::FRAMEBUFFER)};
    if (gl::FRAMEBUFFER_COMPLETE != result) {
        printError("Framebuffer is incomplete.");
        TERMINATE();
    }
    // Switch back to the default framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, DEFAULT_FBO);
}

DeferredRenderer::DeferredRenderer(DeferredRenderer&& dr): m_res_x{dr.m_res_x}, m_res_y{dr.m_res_y},
                  m_sp_osm{std::move(dr.m_sp_osm)}, m_sp_gbuf{std::move(dr.m_sp_gbuf)},
                  m_sp_shade{std::move(dr.m_sp_shade)}, m_hal_tbo{std::move(dr.m_hal_tbo)},
                  m_uniform_mngr{std::move(dr.m_uniform_mngr)}, m_ppl_OSM{std::move(dr.m_ppl_OSM)},
                  m_vpl_OSM{std::move(dr.m_vpl_OSM)}, m_defer_fbo_handle{dr.m_defer_fbo_handle},
                  m_depth_rbo_handle{dr.m_depth_rbo_handle},
                  m_ss_quad_va{std::move(dr.m_ss_quad_va)},
                  m_tex_accum{std::move(m_tex_accum)},
                  m_tex_w_pos{std::move(dr.m_tex_w_pos)},
                  m_tex_w_norm{std::move(dr.m_tex_w_norm)},
                  m_tex_mat_id{std::move(dr.m_tex_mat_id)} {
    // Mark as moved
    dr.m_defer_fbo_handle = 0;
}

DeferredRenderer& DeferredRenderer::operator=(DeferredRenderer&& dr) {
    assert(this != &dr);
    // Free memory
    gl::DeleteRenderbuffers(1, &m_depth_rbo_handle);
    gl::DeleteFramebuffers(1, &m_defer_fbo_handle);
    // Now copy the data
    memcpy(this, &dr, sizeof(*this));
    // Mark as moved
    dr.m_defer_fbo_handle = 0;
    return *this;
}

DeferredRenderer::~DeferredRenderer() {
    // Check if it was moved
    if (m_defer_fbo_handle) {
        gl::DeleteRenderbuffers(1, &m_depth_rbo_handle);
        gl::DeleteFramebuffers(1, &m_defer_fbo_handle);
    }
}

const GLSLProgram& DeferredRenderer::getShadingProgram() const {
    return m_sp_shade;
}

const GLSLProgram& DeferredRenderer::getGBufProgram() const {
    return m_sp_gbuf;
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
    m_sp_osm.use();
    // Set culling & depth testing
    gl::CullFace(gl::FRONT);
    gl::Enable(gl::DEPTH_TEST);
    // Render
    m_ppl_OSM.generate(scene, ppls, model_mat);
    if (settings.gi_enabled) m_vpl_OSM.generate(scene, vpls, model_mat);
}

void DeferredRenderer::generateGBuffer(const Scene& scene) const {
    m_sp_gbuf.use();
    // Clear framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, m_defer_fbo_handle);
    gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
    // Set culling, depth testing & viewport
    gl::CullFace(gl::BACK);
    gl::Enable(gl::DEPTH_TEST);
    gl::Viewport(0, 0, m_res_x, m_res_y);
    // Render
    scene.render();
}

void DeferredRenderer::shade(const int tri_buf_idx) const {
    m_sp_shade.use();
    // Set dynamic uniforms
    m_uniform_mngr.setUniformValues(settings.gi_enabled, settings.clamp_r_sq,
                                    settings.exposure, settings.frame_num,
                                    settings.gi_enabled ? settings.max_num_vpls : 0,
                                    settings.sca_k, settings.abs_k + settings.sca_k,
                                    settings.sca_k / (settings.abs_k + settings.sca_k),
                                    tri_buf_idx);
    // Clear framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, DEFAULT_FBO);
    gl::Clear(gl::COLOR_BUFFER_BIT);
    // Set culling, depth testing & viewport
    gl::CullFace(gl::BACK);
    gl::Disable(gl::DEPTH_TEST);
    gl::Viewport(0, 0, m_res_x, m_res_y);
    // Render
    m_ss_quad_va.draw(gl::TRIANGLE_STRIP);
}
