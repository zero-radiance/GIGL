#pragma once

#include "LightArray.h"
#include <GLM\mat4x4.hpp>
#include "..\GL\GLPersistentBuffer.hpp"

template <class PL>
LightArray<PL>::LightArray(const int max_n_vpls): m_ubo{3 * max_n_vpls * sizeof(PL)},
                                                  m_capacity{max_n_vpls}, m_sz{0}, m_offset{0} {}

template <class PL>
int LightArray<PL>::size() const {
    return m_sz;
}

template <class PL>
PL& LightArray<PL>::operator[](const int index) {
    assert(index < m_sz);
    PL* const pls{data()};
    return pls[index];
}

template <class PL>
const PL& LightArray<PL>::operator[](const int index) const {
    assert(index < m_sz);
    const PL* const pls{data()};
    return pls[index];
}

template <class PL>
void LightArray<PL>::clear() {
    m_sz = 0;
}

template <class PL>
bool LightArray<PL>::isEmpty() const {
    return (0 == m_sz);
}

template <class PL>
void LightArray<PL>::addLight(const PL& pl) {
    PL* pls{data()};
    pls[m_sz++] = pl;
}

template <class PL>
void LightArray<PL>::normalizeIntensity(const int n_paths) {
    assert(m_sz > 0);
    const float inv_n_paths{1.0f / n_paths};
    PL* pls{data()};
    // Remove gaps in path indices
    uint old_path_id{pls[0].m_path_id};
    uint new_path_id{0};
    for (int i = 0; i < m_sz; ++i) {
        if (old_path_id == pls[i].m_path_id) {
            // Same path
            pls[i].m_path_id = new_path_id;
        } else {
            // New path started
            old_path_id = pls[i].m_path_id;
            pls[i].m_path_id = ++new_path_id;
        }
        // Normalize
        pls[i].m_intensity *= inv_n_paths;
    }
}

template <class PL>
void LightArray<PL>::bind(const GLuint bind_idx) const {
    m_ubo.bind(bind_idx);
}

template <class PL>
void LightArray<PL>::switchToNextBuffer() {
    m_offset = (m_offset + m_capacity) % (3 * m_capacity);
}

template <class PL>
PL* LightArray<PL>::data() {
    PL* data_ptr{static_cast<PL*>(m_ubo.data())};
    // Apply the offset
    return data_ptr + m_offset;
}

template <class PL>
const PL* LightArray<PL>::data() const {
    const PL* data_ptr{static_cast<const PL*>(m_ubo.data())};
    // Apply the offset
    return data_ptr + m_offset;
}
