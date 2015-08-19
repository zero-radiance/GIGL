#include "OmniShadowMap.h"
#include <utility>
#include <GLM\gtc\matrix_transform.hpp>
#include <OpenGL\gl_core_4_4.hpp>
#include "..\Common\Constants.h"
#include "..\Common\Utility.hpp"

using glm::vec3;
using glm::mat4;
using glm::min;

CONSTEXPR GLuint targets[] = {gl::TEXTURE_CUBE_MAP_POSITIVE_X,
                              gl::TEXTURE_CUBE_MAP_NEGATIVE_X,
                              gl::TEXTURE_CUBE_MAP_POSITIVE_Y,
                              gl::TEXTURE_CUBE_MAP_NEGATIVE_Y,
                              gl::TEXTURE_CUBE_MAP_POSITIVE_Z,
                              gl::TEXTURE_CUBE_MAP_NEGATIVE_Z};

static inline mat4 calcCubeViewMat(const int face) {
    switch (face) {
        case 0: // +X
            return glm::lookAt(vec3{0}, vec3{ 1, 0, 0}, vec3{0, -1,  0});
        case 1: // -X
            return glm::lookAt(vec3{0}, vec3{-1, 0, 0}, vec3{0, -1,  0});
        case 2: // +Y
            return glm::lookAt(vec3{0}, vec3{0,  1, 0}, vec3{0,  0,  1});
        case 3: // -Y
            return glm::lookAt(vec3{0}, vec3{0, -1, 0}, vec3{0,  0, -1});
        case 4: // +Z
            return glm::lookAt(vec3{0}, vec3{0, 0,  1}, vec3{0, -1,  0});
        case 5: // -Z
            return glm::lookAt(vec3{0}, vec3{0, 0, -1}, vec3{0, -1,  0});
        default:
            printError("Attempting to access non-existing cube face.");
            TERMINATE();
    }
}

OmniShadowMap::OmniShadowMap(const GLsizei res, const GLsizei max_vpls, const float max_dist,
                             const int tex_unit): m_tex_unit{tex_unit}, m_res{res},
                             m_max_vpls{max_vpls}, m_inv_max_dist_sq{1.0f / sq(max_dist)} {
    gl::Enable(gl::TEXTURE_CUBE_MAP_SEAMLESS);
    createDepthTexture();
    createFramebuffer();
    // Set up view-projection matrices
    const mat4 proj_mat{glm::perspective(0.5f * PI, 1.0f, 0.1f, max_dist)};
    for (int f = 0; f < 6; ++f) {
        m_view_proj[f] = proj_mat * calcCubeViewMat(f);
    }
    
}

OmniShadowMap::OmniShadowMap(const OmniShadowMap& osm): m_view_proj(osm.m_view_proj),
               m_tex_unit{osm.m_tex_unit}, m_res{osm.m_res}, m_max_vpls{osm.m_max_vpls},
               m_inv_max_dist_sq{osm.m_inv_max_dist_sq} {
    createDepthTexture();
    createFramebuffer();
}

OmniShadowMap& OmniShadowMap::operator=(const OmniShadowMap& osm) {
    if (this != &osm) {
        m_view_proj       = osm.m_view_proj;
        m_tex_unit        = osm.m_tex_unit;
        m_res             = osm.m_res;
        m_max_vpls        = osm.m_max_vpls;
        m_inv_max_dist_sq = osm.m_inv_max_dist_sq;
        createDepthTexture();
        createFramebuffer();
    }
    return *this;
}

OmniShadowMap::OmniShadowMap(OmniShadowMap&& osm): m_view_proj(std::move(osm.m_view_proj)),
               m_tex_unit{osm.m_tex_unit}, m_res{osm.m_res}, m_max_vpls{osm.m_max_vpls},
               m_inv_max_dist_sq{osm.m_inv_max_dist_sq}, m_fbo_handle{osm.m_fbo_handle},
               m_cube_tex_handle{osm.m_cube_tex_handle} {
    // Mark as moved
    osm.m_cube_tex_handle = 0;
}

OmniShadowMap& OmniShadowMap::operator=(OmniShadowMap&& osm) {
    assert(this != &osm);
    memcpy(this, &osm, sizeof(*this));
    // Mark as moved
    osm.m_cube_tex_handle = 0;
    return *this;
}

OmniShadowMap::~OmniShadowMap() {
    // Check if it was moved
    if (m_cube_tex_handle) {
        gl::DeleteFramebuffers(1, &m_fbo_handle);
        gl::DeleteTextures(1, &m_cube_tex_handle);
    }
}

void OmniShadowMap::createDepthTexture() {
    gl::ActiveTexture(gl::TEXTURE0 + m_tex_unit);
    // Allocate texture storage
    gl::GenTextures(1, &m_cube_tex_handle);
    gl::BindTexture(gl::TEXTURE_CUBE_MAP_ARRAY, m_cube_tex_handle);
    gl::TexStorage3D(gl::TEXTURE_CUBE_MAP_ARRAY, 1, gl::DEPTH_COMPONENT24, m_res, m_res,
                     6 * m_max_vpls);
    // No texture filtering
    gl::TexParameteri(gl::TEXTURE_CUBE_MAP_ARRAY, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
    gl::TexParameteri(gl::TEXTURE_CUBE_MAP_ARRAY, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
    // Use edge-clamping for all 3 dimensions
    gl::TexParameteri(gl::TEXTURE_CUBE_MAP_ARRAY, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
    gl::TexParameteri(gl::TEXTURE_CUBE_MAP_ARRAY, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
    gl::TexParameteri(gl::TEXTURE_CUBE_MAP_ARRAY, gl::TEXTURE_WRAP_R, gl::CLAMP_TO_EDGE);
    // Return result of comparison of tex(x, y, z, i) with current distance
    gl::TexParameteri(gl::TEXTURE_CUBE_MAP_ARRAY, gl::TEXTURE_COMPARE_MODE,
                      gl::COMPARE_REF_TO_TEXTURE);
    // curr_dist < tex(x, y, z, i) ? 1.0 : 0.0
    gl::TexParameteri(gl::TEXTURE_CUBE_MAP_ARRAY, gl::TEXTURE_COMPARE_FUNC, gl::LESS);
}

void OmniShadowMap::createFramebuffer() {
    gl::GenFramebuffers(1, &m_fbo_handle);
    gl::BindFramebuffer(gl::FRAMEBUFFER, m_fbo_handle);
    // Attach depth texture to framebuffer
    gl::FramebufferTexture(gl::FRAMEBUFFER, gl::DEPTH_ATTACHMENT, m_cube_tex_handle, 0);
    // Specify color buffers to be drawn to; gl::NONE <- not drawing within fragment shader
    // Double-speed Z prepass
    static const GLenum draw_buffers[] = {gl::NONE};
    gl::DrawBuffers(1, draw_buffers);
    // Verify framebuffer
    const GLenum result{gl::CheckFramebufferStatus(gl::FRAMEBUFFER)};
    if (gl::FRAMEBUFFER_COMPLETE != result) {
        printError("Framebuffer is incomplete.");
        TERMINATE();
    }
    // Switch back to the default framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, DEFAULT_FBO);
}
