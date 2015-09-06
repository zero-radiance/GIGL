#pragma once

#include "DensityField.h"

class Scene;

/* Scalar-valued fog bounded by a rectangular parallelepiped (AABB) */
class FogVolume {
public:
    FogVolume() = delete;
    RULE_OF_ZERO(FogVolume);
    // Constructor, computes the values of density field
    explicit FogVolume(const BBox& bb, const int(&res)[3], const float freq, const float ampl,
                       const float maj_ext_k, const float abs_k,
                       const float sca_k, const PerspectiveCamera& cam, const Scene& scene);
    // Constructor, uses the supplied density field
    explicit FogVolume(DensityField&& df, const float maj_ext_k,
                       const float abs_k, const float sca_k);
    // Modifies absorption, scattering and extinction coefficients
    void setCoeffs(const float maj_ext_k, const float abs_k, const float sca_k);
    // Returns bounding box of fog volume
    const BBox& bbox() const;
    // Returns majorant extinction coefficient
    float getMajExtK() const;
    // Samples extinction coefficient at a given position
    float sampleExtK(const glm::vec3& pos) const;
    // Samples scattering coefficient at a given position
    float sampleScaK(const glm::vec3& pos) const;
    // Returns scattering albedo (constant across the entire volume)
    float getScaAlbedo() const;
    // Returns volume entry and exit distances
    BBox::IntDist intersect(const rt::Ray& ray) const;
private:
    // Private data members
    DensityField m_df;          // Scalar-valued particle density field
    float        m_abs_k;       // Absorption coefficient per unit density
    float        m_sca_k;       // Scattering coefficient per unit density
    float        m_maj_ext_k;   // Max. extinction coefficient present in volume
};
