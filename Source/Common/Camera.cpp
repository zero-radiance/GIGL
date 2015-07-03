#include "Camera.h"
#include <GLM\gtc\matrix_transform.hpp>
#include <GLM\gtc\matrix_inverse.hpp>
#include "..\RT\RTBase.h"

using glm::ivec2;
using glm::vec3;
using glm::mat3;
using glm::mat4;
using glm::cross;
using glm::lookAt;
using glm::normalize;
using glm::perspective;

PerspectiveCamera::PerspectiveCamera(const vec3& world_pos, const vec3& dir, const vec3& up,
                                     const float h_fov, const int res_x, const int res_y,
                                     const float t_near, const float t_far):
                                     m_res{res_x, res_y}, m_world_pos{world_pos} {
    const float aspect_ratio{static_cast<float>(res_x) / static_cast<float>(res_y)};
    // Set up virtual sensor coordinate system
    const vec3 fwd_axis{normalize(dir)};
    const vec3 right_axis{normalize(cross(fwd_axis, up))};
    const vec3 up_axis{normalize(cross(right_axis, fwd_axis))};
    const vec3 row_vec{2.0f * tan(0.5f * h_fov) * right_axis * aspect_ratio};
    const vec3 col_vec{2.0f * tan(0.5f * h_fov) * up_axis};
    m_bottom_left = fwd_axis - 0.5f * row_vec - 0.5f * col_vec;
    // Compute pixel/sensor_element dimension vectors
    m_step_x = row_vec / static_cast<float>(res_x);
    m_step_y = col_vec / static_cast<float>(res_y);
    // Compute the transformation matrices
    m_view_mat = lookAt(world_pos, world_pos + dir, up);
    m_proj_mat = perspective(h_fov, aspect_ratio, t_near, t_far);
}

const glm::ivec2& PerspectiveCamera::resolution() const {
    return m_res;
}

const vec3& PerspectiveCamera::worldPos() const {
    return m_world_pos;
}

const mat4& PerspectiveCamera::viewMat() const {
    return m_view_mat;
}

const mat4& PerspectiveCamera::projMat() const {
    return m_proj_mat;
}

mat4 PerspectiveCamera::computeModelView(const mat4& model_mat) const {
    return m_view_mat * model_mat;
}

mat4 PerspectiveCamera::computeMVP(const mat4& model_view) const {
    return m_proj_mat * model_view;
}

mat3 PerspectiveCamera::computeNormXForm(const mat4& m) {
    // Remove the translation component
    const mat3 m_no_transl = mat3{m};
    return glm::inverseTranspose(m_no_transl);
}

rt::Ray PerspectiveCamera::getPrimaryRay(const float x, const float y) const {
    const vec3 dir{m_bottom_left + x * m_step_x + y * m_step_y};
    return rt::Ray{m_world_pos, normalize(dir)};
}
