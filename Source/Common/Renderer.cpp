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
                  m_tex_w_pos{TEX_U_W_POS, res_x, res_y, false, false},
                  m_tex_w_norm{TEX_U_W_NORM, res_x, res_y, false, false},
                  m_tex_mat_id{TEX_U_MAT_ID, res_x, res_y, false, false},
                  m_tex_fog_dist{TEX_U_FOG_DIST, res_x, res_y, false, false},
                  m_tex_vol_comp{TEX_U_VOL_COMP, res_x / 2, res_y / 2, false, true} {
    // Generate a Halton sequence for 30 frames with (up to) 24 samples per frame
    static const GLuint seq_sz{30 * 24};
    GLfloat hal_seq[seq_sz];
    HaltonSG::generate<2>(hal_seq);
    m_hal_tbo.bufferData(sizeof(hal_seq), hal_seq);
    // Load the required shaders
    loadShaders();
    // Manage the following uniforms automatically
    m_uni_mngr_surf.setManagedUniforms(m_sp_shade_surface, {"gi_enabled", "clamp_rsq",
                                                            "frame_id", "n_vpls", "ext_k",
                                                            "sca_albedo", "tri_buf_idx"});
    m_uni_mngr_vol.setManagedUniforms(m_sp_shade_volume, {"gi_enabled", "clamp_rsq",
                                                          "frame_id", "n_vpls", "sca_k", "ext_k",
                                                          "sca_albedo", "tri_buf_idx"});
    m_uni_mngr_combine.setManagedUniforms(m_sp_combine, {"exposure", "frame_id", "ext_k"});
    // Create a screen space quad
    CONSTEXPR float ss_quad_pos[] = {-1.0f, -1.0f, 0.0f,    // Bottom left
                                      1.0f, -1.0f, 0.0f,    // Bottom right
                                     -1.0f,  1.0f, 0.0f,    // Top left
                                      1.0f,  1.0f, 0.0f};   // Top right
    m_ss_quad_va.loadData(0, 12, ss_quad_pos);
    m_ss_quad_va.buffer();
    // Bind textures to image units
    gl::BindImageTexture(IMG_U_ACCUM, m_tex_accum.id(),
                         0, false, 0, gl::READ_WRITE, gl::RGBA32F);
    gl::BindImageTexture(IMG_U_FOG_DIST, m_tex_fog_dist.id(),
                         0, false, 0, gl::READ_WRITE, gl::RG32F);
    // Generate framebuffers
    generateDeferredFBO();
    generateVolumeFBO();
}

void DeferredRenderer::loadShaders() {
    // Load shadow map generating program
    m_sp_osm.loadShader("Source\\Shaders\\Shadow.vert");
    m_sp_osm.loadShader("Source\\Shaders\\Shadow.geom");
    m_sp_osm.loadShader("Source\\Shaders\\Shadow.frag");
    m_sp_osm.link();
    // Load shaders which fill the G-buffer
    m_sp_gbuf.loadShader("Source\\Shaders\\GBuffer.vert");
    m_sp_gbuf.loadShader("Source\\Shaders\\GBuffer.frag");
    m_sp_gbuf.link();
    // Load shaders which perform surface shading
    m_sp_shade_surface.loadShader("Source\\Shaders\\Shade.vert");
    m_sp_shade_surface.loadShader("Source\\Shaders\\Surface.frag");
    m_sp_shade_surface.link();
    // Load shaders which perform volume shading
    m_sp_shade_volume.loadShader("Source\\Shaders\\Shade.vert");
    m_sp_shade_volume.loadShader("Source\\Shaders\\Volume.frag");
    m_sp_shade_volume.link();
    // Load shaders which combine the results of surface and volume shading
    m_sp_combine.loadShader("Source\\Shaders\\Shade.vert");
    m_sp_combine.loadShader("Source\\Shaders\\Combine.frag");
    m_sp_combine.link();
}

void DeferredRenderer::generateDeferredFBO() {
    // Generate and bind the FBO
    gl::GenFramebuffers(1, &m_defer_fbo_handle);
    gl::BindFramebuffer(gl::FRAMEBUFFER, m_defer_fbo_handle);
    // Generate and bind the depth buffer
    gl::GenRenderbuffers(1, &m_depth_rbo_handle);
    gl::BindRenderbuffer(gl::RENDERBUFFER, m_depth_rbo_handle);
    gl::RenderbufferStorage(gl::RENDERBUFFER, gl::DEPTH_COMPONENT, m_res_x, m_res_y);
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
    const GLenum draw_buffers[] = {gl::COLOR_ATTACHMENT0, gl::COLOR_ATTACHMENT1, gl::COLOR_ATTACHMENT2};
    gl::DrawBuffers(3, draw_buffers);
    // Verify the framebuffer
    const GLenum result{gl::CheckFramebufferStatus(gl::FRAMEBUFFER)};
    if (gl::FRAMEBUFFER_COMPLETE != result) {
        printError("Framebuffer is incomplete.");
        TERMINATE();
    }
}

void DeferredRenderer::generateVolumeFBO() {
    // Generate and bind the FBO
    gl::GenFramebuffers(1, &m_vol_fbo_handle);
    gl::BindFramebuffer(gl::FRAMEBUFFER, m_vol_fbo_handle);
    // Attach the texture to the framebuffer
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D,
                             m_tex_vol_comp.id(), 0);
    // Specify the buffer to draw to
    const GLenum draw_buffers[] = {gl::COLOR_ATTACHMENT0};
    gl::DrawBuffers(1, draw_buffers);
    // Verify the framebuffer
    const GLenum result{gl::CheckFramebufferStatus(gl::FRAMEBUFFER)};
    if (gl::FRAMEBUFFER_COMPLETE != result) {
        printError("Framebuffer is incomplete.");
        TERMINATE();
    }
}

DeferredRenderer::DeferredRenderer(DeferredRenderer&& dr): settings(dr.settings),
                  m_res_x{dr.m_res_x}, m_res_y{dr.m_res_y},
                  m_sp_osm{std::move(dr.m_sp_osm)}, m_sp_gbuf{std::move(dr.m_sp_gbuf)},
                  m_sp_shade_surface{std::move(dr.m_sp_shade_surface)},
                  m_sp_shade_volume{std::move(dr.m_sp_shade_volume)},
                  m_sp_combine{std::move(dr.m_sp_combine)},
                  m_hal_tbo{std::move(dr.m_hal_tbo)},
                  m_uni_mngr_surf{std::move(dr.m_uni_mngr_surf)},
                  m_uni_mngr_vol{std::move(dr.m_uni_mngr_vol)},
                  m_uni_mngr_combine{std::move(dr.m_uni_mngr_combine)},
                  m_ppl_OSM{std::move(dr.m_ppl_OSM)}, m_vpl_OSM{std::move(dr.m_vpl_OSM)},
                  m_defer_fbo_handle{dr.m_defer_fbo_handle},
                  m_depth_rbo_handle{dr.m_depth_rbo_handle},
                  m_vol_fbo_handle{dr.m_vol_fbo_handle},
                  m_ss_quad_va{std::move(dr.m_ss_quad_va)},
                  m_tex_accum{std::move(m_tex_accum)},
                  m_tex_w_pos{std::move(dr.m_tex_w_pos)},
                  m_tex_w_norm{std::move(dr.m_tex_w_norm)},
                  m_tex_mat_id{std::move(dr.m_tex_mat_id)},
                  m_tex_fog_dist{std::move(dr.m_tex_fog_dist)},
                  m_tex_vol_comp{std::move(dr.m_tex_vol_comp)} {
    // Mark as moved
    dr.m_defer_fbo_handle = 0;
}

DeferredRenderer& DeferredRenderer::operator=(DeferredRenderer&& dr) {
    assert(this != &dr);
    // Free memory
    gl::DeleteFramebuffers(1, &m_vol_fbo_handle);
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

const GLSLProgram& DeferredRenderer::gBufferSP() const {
    return m_sp_gbuf;
}

const GLSLProgram& DeferredRenderer::surfaceSP() const {
    return m_sp_shade_surface;
}

const GLSLProgram& DeferredRenderer::volumeSP() const {
    return m_sp_shade_volume;
}

const GLSLProgram& DeferredRenderer::combineSP() const {
    return m_sp_combine;
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
    // Set culling and depth testing
    gl::CullFace(gl::FRONT);
    gl::Enable(gl::DEPTH_TEST);
    // Add an offset to the depth value of each polygon
    gl::Enable(gl::POLYGON_OFFSET_FILL);
    gl::PolygonOffset(1.1f, 4.0f);
    // Render
    m_ppl_OSM.generate(scene, ppls, model_mat);
    if (settings.gi_enabled) m_vpl_OSM.generate(scene, vpls, model_mat);
    // Disable depth offsetting
    gl::Disable(gl::POLYGON_OFFSET_FILL);
}

void DeferredRenderer::generateGBuffer(const Scene& scene) const {
    // Bind and clear the deferred framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, m_defer_fbo_handle);
    gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
    // Set culling, depth test and viewport
    gl::CullFace(gl::BACK);
    gl::Enable(gl::DEPTH_TEST);
    gl::Viewport(0, 0, m_res_x, m_res_y);
    // Install the shader program
    m_sp_gbuf.use();
    // Render
    scene.render();
}

void DeferredRenderer::shade(const int tri_buf_idx) const {
    gl::Disable(gl::DEPTH_TEST);
    /* Perform surface shading */
    m_sp_shade_surface.use();
    // Set dynamic uniforms
    m_uni_mngr_surf.setUniformValues(settings.gi_enabled, settings.clamp_r_sq,
                                     settings.frame_num, settings.max_num_vpls,
                                     settings.abs_k + settings.sca_k,
                                     settings.sca_k / (settings.abs_k + settings.sca_k),
                                     tri_buf_idx);
    // Bind and clear the display framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, DEFAULT_FBO);
    gl::Clear(gl::COLOR_BUFFER_BIT);
    // Render surfaces at the full resolution
    gl::Viewport(0, 0, m_res_x, m_res_y);
    m_ss_quad_va.draw(gl::TRIANGLE_STRIP);
    // Check if there is fog to render
    if (settings.abs_k + settings.sca_k > 0.0f) {
        /* Perform volume shading */
        m_sp_shade_volume.use();
        // Set dynamic uniforms
        m_uni_mngr_vol.setUniformValues(settings.gi_enabled, settings.clamp_r_sq,
                                        settings.frame_num, settings.max_num_vpls, settings.sca_k,
                                        settings.abs_k + settings.sca_k,
                                        settings.sca_k / (settings.abs_k + settings.sca_k),
                                        tri_buf_idx);
        // Bind and clear the fog (volume) framebuffer
        gl::BindFramebuffer(gl::FRAMEBUFFER, m_vol_fbo_handle);
        gl::Clear(gl::COLOR_BUFFER_BIT);
        // Render fog at the quarter of the full resolution
        gl::Viewport(0, 0, m_res_x / 2, m_res_y / 2);
        m_ss_quad_va.draw(gl::TRIANGLE_STRIP);
    }
    /* Combine surface and volume shading results */
    m_sp_combine.use();
    m_uni_mngr_combine.setUniformValues(settings.exposure, settings.frame_num,
                                        settings.abs_k + settings.sca_k);
    // Bind and clear the display framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, DEFAULT_FBO);
    gl::Clear(gl::COLOR_BUFFER_BIT);
    // Create full resolution screen output
    gl::Viewport(0, 0, m_res_x, m_res_y);
    m_ss_quad_va.draw(gl::TRIANGLE_STRIP);
}
