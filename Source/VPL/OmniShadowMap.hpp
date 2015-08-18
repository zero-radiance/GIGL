#pragma once

#include "OmniShadowMap.h"
#include <GLM\gtc\matrix_transform.hpp>
#include "..\Common\Scene.h"
#include "LightArray.hpp"

template <class PL>
void OmniShadowMap::generate(const Scene& scene, const LightArray<PL>& la,
                             const glm::mat4& model_mat) const {
    // Set non-light-specific parameters
    gl::BindFramebuffer(gl::FRAMEBUFFER, m_fbo_handle);
    gl::Clear(gl::DEPTH_BUFFER_BIT);
    gl::Viewport(0, 0, m_res, m_res);
    gl::UniformMatrix4fv(UL_SM_MODELMAT, 1, GL_FALSE, &model_mat[0][0]);
    gl::Uniform1f(UL_SM_INVMAXD2, m_inv_max_dist_sq);
    assert(la.size() <= m_max_vpls);
    const GLsizei n{la.size()};
    for (GLsizei i = 0; i < n; ++i) {
        const glm::vec3& light_w_pos{la[i].wPos()};
        const glm::mat4  light_inv_trans{glm::translate(glm::mat4{1.0f}, -light_w_pos)};
        // Set light source position
        gl::Uniform3f(UL_SM_WPOS_VPL, light_w_pos.x, light_w_pos.y, light_w_pos.z);
        // Set light transformation matrix
        const glm::mat4 lightMVPs[6] = {{m_view_proj[0] * light_inv_trans},
                                        {m_view_proj[1] * light_inv_trans},
                                        {m_view_proj[2] * light_inv_trans},
                                        {m_view_proj[3] * light_inv_trans},
                                        {m_view_proj[4] * light_inv_trans},
                                        {m_view_proj[5] * light_inv_trans}};
        gl::UniformMatrix4fv(UL_SM_LIGHTMVP, 6, GL_FALSE, &lightMVPs[0][0][0]);
        // Set layer index
        gl::Uniform1i(UL_SM_LAYER_ID, 6 * i);
        // Render scene to depth map
        scene.render(true);
    }
}
