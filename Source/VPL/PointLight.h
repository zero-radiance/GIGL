#pragma once

#include <GLM\vec3.hpp>
#include <GLM\vec4.hpp>
#include "..\Common\Definitions.h"

template <class PL> class LightArray;

/* Base class for Point Lights */
template <class T>
class PointLight {
public:
    PointLight() = default;
    RULE_OF_ZERO(PointLight);
    // Returns light position in world coordinates
    const glm::vec3& wPos() const;
    // Returns light intensity
    const glm::vec3& intensity() const;
    // Sets specified light position in world coordinates
    const void setWPos(const glm::vec3& w_pos);
    // Sets specified light intensity
    const void setIntensity(const glm::vec3& intensity);
};

/* Primary Point Light */
class PPL: public PointLight<PPL> {
public:
    PPL() = delete;
    RULE_OF_ZERO(PPL);
    // Constructs a primary point light with specified position and intensity
    explicit PPL(const glm::vec3& w_pos, const glm::vec3& intensity);
private:
    friend class PointLight<PPL>;
    friend class LightArray<PPL>;
    // Returns light position in world coordinates
    const glm::vec3& getWPos() const;
    // Returns light intensity
    const glm::vec3& getIntensity() const;
    // std140 and std430 compatible storage
    glm::vec3 m_w_pos;		// Position in world space
    uint	  m_type;		// 0 for PPLs
    glm::vec3 m_intensity;	// Light intensity
    uint8_t   slack[4];     // 4 byte padding
};

/* Virtual Point Light */
class VPL: public PointLight<VPL> {
public:
    VPL() = delete;
    RULE_OF_ZERO(VPL);
    // Constructor for VPLs in medium
    explicit VPL(const uint path_id, const glm::vec3& w_pos, const glm::vec3& w_inc,
                 const glm::vec3& intensity, const float sca_k);
    // Constructor for VPLs on surfaces
    explicit VPL(const uint path_id, const glm::vec3& w_pos, const glm::vec3& w_inc,
                 const glm::vec3& intensity, const glm::vec3& w_norm,
                 const glm::vec3& k_d, const glm::vec3& k_s, const float n_s);
private:
    friend class PointLight<VPL>;
    friend class LightArray<VPL>;
    // Returns light position in world coordinates
    const glm::vec3& getWPos() const;
    // Returns light intensity
    const glm::vec3& getIntensity() const;
    // std140 and std430 compatible storage
    glm::vec3 m_w_pos;		// Position in world space
    uint	  m_type;		// 1 = VPL in volume, 2 = VPL on surface
    glm::vec3 m_intensity;	// Light intensity
    float	  m_sca_k;      // Scattering coefficient             | Volume only
    glm::vec3 m_w_inc;		// Incoming direction in world space  | Volume and Surface
    uint	  m_path_id;	// Path index                         | Volume and Surface
    glm::vec3 m_w_norm;     // Normal direction in world space    | Surface only
    uint8_t   pad_1[4];     // 4 byte padding
    glm::vec3 m_k_d;        // Diffuse coefficient                | Surface only
    uint8_t   pad_2[4];     // 4 byte padding
    glm::vec3 m_k_s;        // Specular coefficient               | Surface only
    float	  m_n_s;        // Specular exponent                  | Surface only
};
