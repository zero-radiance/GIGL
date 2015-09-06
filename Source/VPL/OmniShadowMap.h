#pragma once

#include <array>
#include <OpenGL\gl_basic_typedefs.h>
#include <GLM\mat4x4.hpp>
#include "..\Common\Definitions.h"

class Scene;
template <class PL> class LightArray;

class OmniShadowMap {
public:
    OmniShadowMap() = delete;
    RULE_OF_FIVE(OmniShadowMap);
    // Constructor
    // res:      shadow map map texture resolution: res x res
    // max_vpls: maximal number of Virtual Point Lights
    // max_dist: maximal distance at which shadow is still being cast
    // tex_unit: OpenGL texture unit id
    OmniShadowMap(const GLsizei res, const GLsizei max_vpls, const float max_dist,
                  const int tex_unit);
    // Runs a rendering pass to generate shadow map
    template <class PL>
    void generate(const Scene& scene, const LightArray<PL>& la, const glm::mat4& model_mat) const;
private:
    // Creates a cubemap array depth texture
    void createDepthTexture();
    // Creates a framebuffer for depth map rendering
    void createFramebuffer();
    // Private data members
    std::array<glm::mat4, 6> m_view_proj;        // Projection * View matrices, 1 for each face
    int                      m_tex_unit;         // OpenGL texture unit id
    GLsizei                  m_res;              // Shadow map texture resolution: res x res
    GLsizei                  m_max_vpls;         // Maximal number of Virtual Point Lights
    float                    m_inv_max_dist_sq;  // Inverse max. [shadow] distance squared
    GLuint                   m_fbo_handle;       // OpenGL frame buffer object handle
    GLuint                   m_cube_tex_handle;  // OpenGL cubemap array texture handle
};
