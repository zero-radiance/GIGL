#include "GLElementBuffer.h"
#include <cassert>
#include <OpenGL\gl_core_4_4.hpp>
#include <GLM\detail\func_common.hpp>
#include "GLVertArray.h"

using glm::min;
using glm::max;

GLElementBuffer::GLElementBuffer(): m_min_idx{UINT_MAX}, m_max_idx{0}, m_is_buffered{false} {
    gl::GenBuffers(1, &m_handle);
}

GLElementBuffer::GLElementBuffer(const GLElementBuffer& ebo): m_min_idx{ebo.m_min_idx},
                 m_max_idx{ebo.m_max_idx}, m_is_buffered{ebo.m_is_buffered},
                 m_data_vec(ebo.m_data_vec) {
    gl::GenBuffers(1, &m_handle);
    if (ebo.m_is_buffered) { buffer(); }
}

GLElementBuffer& GLElementBuffer::operator=(const GLElementBuffer& ebo) {
    if (this != &ebo) {
        // Free memory
        gl::DeleteBuffers(1, &m_handle);
        // Generate a new buffer
        gl::GenBuffers(1, &m_handle);
        // Copy the data
        m_min_idx     = ebo.m_min_idx;
        m_max_idx     = ebo.m_max_idx;
        m_is_buffered = ebo.m_is_buffered;
        m_data_vec    = ebo.m_data_vec;
        if (ebo.m_is_buffered) { buffer(); }
    }
    return *this;
}

GLElementBuffer::GLElementBuffer(GLElementBuffer&& ebo): m_handle{ebo.m_handle},
                 m_min_idx{ebo.m_min_idx}, m_max_idx{ebo.m_max_idx},
                 m_is_buffered{ebo.m_is_buffered}, m_data_vec(ebo.m_data_vec) {
    // Mark as moved
    ebo.m_handle = 0;
}

GLElementBuffer& GLElementBuffer::operator=(GLElementBuffer&& ebo) {
    assert(this != &ebo);
    // Free memory
    gl::DeleteBuffers(1, &m_handle);
    // Now copy the data
    memcpy(this, &ebo, sizeof(*this));
    // Mark as moved
    ebo.m_handle = 0;
    return *this;
}

GLElementBuffer::~GLElementBuffer() {
    // Check if it was moved
    if (m_handle) {
        gl::DeleteBuffers(1, &m_handle);
    }
}

void GLElementBuffer::buffer() {
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, m_handle);
    const auto byte_sz = m_data_vec.size() * sizeof(GLuint);
    gl::BufferData(gl::ELEMENT_ARRAY_BUFFER, byte_sz, m_data_vec.data(), gl::STATIC_DRAW);
    m_is_buffered = true;
}

void GLElementBuffer::loadData(const std::vector<GLuint>& data_vec, const GLuint offset) {
    loadData(data_vec.size(), data_vec.data(), offset);
}

void GLElementBuffer::loadData(const size_t n_elems, const GLuint* const data,
                               const GLuint offset) {
    for (auto i = 0; i < n_elems; ++i) {
        const GLuint idx{offset + data[i]};
        m_min_idx = min(m_min_idx, idx);
        m_max_idx = max(m_max_idx, idx);
        m_data_vec.push_back(idx);
    }
    m_is_buffered = false;
}

void GLElementBuffer::draw(const GLVertArray& va) const {
    gl::BindVertexArray(va.id());
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, m_handle);
    const auto n_elems = static_cast<GLsizei>(m_data_vec.size());
    gl::DrawRangeElements(gl::TRIANGLES, m_min_idx, m_max_idx, n_elems, gl::UNSIGNED_INT, nullptr);
}
