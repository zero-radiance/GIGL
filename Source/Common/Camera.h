#pragma once

#include <GLM\vec2.hpp>
#include <GLM\vec3.hpp>
#include <GLM\mat4x4.hpp>
#include "Definitions.h"

namespace rt { class Ray; }

/* Perspective camera class */
class PerspectiveCamera {
public:
    PerspectiveCamera() = delete;
    RULE_OF_ZERO(PerspectiveCamera);
    // Constructs camera using specified world space position, direction (focal axis), up vector,
    // horizontal FoV, resolution, and distances to near and far planes
    explicit PerspectiveCamera(const glm::vec3& world_pos, const glm::vec3& dir,
                               const glm::vec3& up, const float h_fov,
                               const int res_x, const int res_y,
                               const float t_near, const float t_far);
    // Returns sensor resolution
    const glm::ivec2& resolution() const;
    // Returns camera position in world space
    const glm::vec3& worldPos() const;
    // Returns matrix transforming from world to camera space
    const glm::mat4& viewMat() const;
    // Returns matrix transforming from camera to to homogeneous (projected) coordinates
    const glm::mat4& projMat() const;
    // Computes model-view matrix given model matrix
    glm::mat4 computeModelView(const glm::mat4& model_mat) const;
    // Computes model-view-projection matrix given model-view matrix
    glm::mat4 computeMVP(const glm::mat4& model_view) const;
    // Computes normal transformation matrix
    static glm::mat3 computeNormXForm(const glm::mat4& model_view);
    // Returns primary ray for specified screen space coordinates
    // No pixel center adjustments!
    rt::Ray getPrimaryRay(const float x, const float y) const;
private:
    glm::ivec2 m_res;           // Sensor resolution
    glm::vec3  m_world_pos;     // Position in World space
    glm::vec3  m_bottom_left;   // Bottom left corner of the virtual sensor
    glm::vec3  m_step_x;        // Horizontal dimensions of virtual sensor element
    glm::vec3  m_step_y;        // Vertical dimensions of virtual sensor element
    glm::mat4  m_view_mat;      // World to Camera space transformation matrix
    glm::mat4  m_proj_mat;      // Camera space to Homogeneous space transformation matrix
};
