#include "FogVolume.h"
#include <utility>

using glm::vec3;


FogVolume::FogVolume(const BBox& bb, const int (&res)[3], const float freq, const float ampl,
                     const float maj_ext_k, const float abs_k, const float sca_k,
                     const PerspectiveCamera& cam, const Scene& scene):
                     m_df{bb, res, freq, ampl, cam, scene}, m_abs_k{abs_k}, m_sca_k{sca_k},
                     m_maj_ext_k{maj_ext_k} {}

FogVolume::FogVolume(DensityField&& df, const float maj_ext_k, const float abs_k,
                     const float sca_k): m_df{std::forward<DensityField>(df)}, m_abs_k{abs_k}, m_sca_k{sca_k},
                     m_maj_ext_k{maj_ext_k} {}

void FogVolume::setCoeffs(const float maj_ext_k, const float abs_k,
                          const float sca_k) {
    m_maj_ext_k = maj_ext_k;
    m_abs_k     = abs_k;
    m_sca_k     = sca_k;
}

const BBox& FogVolume::bbox() const {
    return m_df.bbox();
}

float FogVolume::getMajExtK() const {
    return m_maj_ext_k;
}

float FogVolume::sampleExtK(const vec3& pos) const {
    const float dens{m_df.sampleDensity(pos)};
    const float ext_k{m_abs_k + m_sca_k};
    return ext_k * dens;
}

float FogVolume::sampleScaK(const vec3& pos) const {
    const float dens{m_df.sampleDensity(pos)};
    return m_sca_k * dens;
}

float FogVolume::getScaAlbedo() const {
    const float ext_k{m_abs_k + m_sca_k};
    return m_sca_k / ext_k;
}

BBox::IntDist FogVolume::intersect(const rt::Ray& ray) const {
    return m_df.intersect(ray);
}
