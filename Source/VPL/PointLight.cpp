#include "PointLight.h"

PPL::PPL(const glm::vec3& w_pos, const glm::vec3& intensity): m_w_pos{w_pos}, m_type{0},
                                                              m_intensity{intensity} {}


const glm::vec3& PPL::getWPos() const {
    return m_w_pos;
}

const glm::vec3& PPL::getIntensity() const {
    return m_intensity;
}

VPL::VPL(const uint path_id, const glm::vec3& w_pos, const glm::vec3& w_inc, const glm::vec3& intensity,
         const float sca_k): m_w_pos{w_pos}, m_type{1}, m_intensity{intensity}, m_sca_k{sca_k},
                             m_w_inc{w_inc}, m_path_id{path_id} {}

VPL::VPL(const uint path_id, const glm::vec3& w_pos, const glm::vec3& w_inc, const glm::vec3& intensity,
         const glm::vec3& w_norm, const glm::vec3& k_d, const glm::vec3& k_s, const float n_s):
         m_w_pos{w_pos + 1E-4f * w_norm}, m_type{2}, m_intensity{intensity}, m_w_inc{w_inc},
         m_path_id{path_id}, m_w_norm{w_norm}, m_k_d{k_d}, m_k_s{k_s}, m_n_s{n_s} {}

const glm::vec3& VPL::getWPos() const {
    return m_w_pos;
}

const glm::vec3& VPL::getIntensity() const {
    return m_intensity;
}
