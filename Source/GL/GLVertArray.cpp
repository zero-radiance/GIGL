#include "GLVertArray.h"
#include <cassert>
#include <OpenGL\gl_core_4_4.hpp>
#include <GLM\detail\func_common.hpp>
#include "..\Common\Constants.h"

using glm::min;
using glm::max;

GLVertArray::GLVertArray(const GLuint n_attr, const GLsizei* const component_counts):
                         m_n_vbos{n_attr}, m_vbos{new VertBuffer[n_attr]},
                         m_ibo{nullptr}, m_num_vert{0}, m_is_buffered{false} {
    // Create vertex array object
    gl::GenVertexArrays(1, &m_handle);
    gl::BindVertexArray(m_handle);
    // Create buffer objects
    GLuint buffer_handles[MAX_N_VBOS];
    gl::GenBuffers(m_n_vbos, buffer_handles);
    for (auto i = 0; i < m_n_vbos; ++i) {
        // Assign handles
        m_vbos[i].handle = buffer_handles[i];
        // Assign strides
        m_vbos[i].n_components = component_counts[i];
        // Reserve a bit of memory
        m_vbos[i].data_vec.reserve(12);
    }
    // Map attribute incides to buffers
    for (GLuint i = 0; i < static_cast<GLuint>(m_n_vbos); ++i) {
        gl::EnableVertexAttribArray(i);
        #ifdef OLD_STYLE_BINDING
            gl::BindBuffer(gl::ARRAY_BUFFER, m_vbos[i].handle);
            gl::VertexAttribPointer(i, m_vbos[i].n_components, gl::FLOAT, GL_FALSE, 0, nullptr);
        #else
            gl::BindVertexBuffer(i, m_vbos[i].handle, 0, m_vbos[i].n_components * sizeof(GLfloat));
            gl::VertexAttribFormat(i, m_vbos[i].n_components, gl::FLOAT, GL_FALSE, 0);
            gl::VertexAttribBinding(i, i);
        #endif
    }
}

GLVertArray::GLVertArray(const GLVertArray& va): m_n_vbos{va.m_n_vbos},
                                                 m_vbos{new VertBuffer[m_n_vbos]},
                                                 m_ibo{va.isIndexed() ? new IndexBuffer : nullptr},
                                                 m_num_vert{va.m_num_vert},
                                                 m_is_buffered{va.m_is_buffered} {
    // Create vertex array object
    gl::GenVertexArrays(1, &m_handle);
    gl::BindVertexArray(m_handle);
    // Create buffer objects
    GLuint buffer_handles[MAX_N_VBOS];
    gl::GenBuffers(m_n_vbos, buffer_handles);
    for (auto i = 0; i < m_n_vbos; ++i) {
        // Use the new handles...
        m_vbos[i].handle = buffer_handles[i];
        // ... and copy the rest
        m_vbos[i].n_components = va.m_vbos[i].n_components;
        m_vbos[i].data_vec     = va.m_vbos[i].data_vec;
    }
    // Map attribute incides to buffers
    for (GLuint i = 0; i < static_cast<GLuint>(m_n_vbos); ++i) {
        gl::EnableVertexAttribArray(i);
        #ifdef OLD_STYLE_BINDING
             gl::BindBuffer(gl::ARRAY_BUFFER, m_vbos[i].handle);
             gl::VertexAttribPointer(i, m_vbos[i].n_components, gl::FLOAT, GL_FALSE, 0, nullptr);
        #else
             gl::BindVertexBuffer(i, m_vbos[i].handle, 0, m_vbos[i].n_components * sizeof(GLfloat));
             gl::VertexAttribFormat(i, m_vbos[i].n_components, gl::FLOAT, GL_FALSE, 0);
             gl::VertexAttribBinding(i, i);
        #endif
    }
    if (va.isIndexed()) {
        // Generate a new handle...
        gl::GenBuffers(1, &m_ibo->handle);
        // ... and copy the data
        m_ibo->data_vec = va.m_ibo->data_vec;
    }
    if (va.m_is_buffered) { buffer(); }
}

GLVertArray& GLVertArray::operator=(const GLVertArray& va) {
    if (this != &va) {
        m_n_vbos      = va.m_n_vbos;
        m_vbos        = new VertBuffer[m_n_vbos];
        m_ibo         = va.isIndexed() ? new IndexBuffer : nullptr;
        m_num_vert    = va.m_num_vert;
        m_is_buffered = va.m_is_buffered;
        // Create vertex array object
        gl::GenVertexArrays(1, &m_handle);
        gl::BindVertexArray(m_handle);
        // Create buffer objects
        GLuint buffer_handles[MAX_N_VBOS];
        gl::GenBuffers(m_n_vbos, buffer_handles);
        for (auto i = 0; i < m_n_vbos; ++i) {
            // Use the new handles...
            m_vbos[i].handle = buffer_handles[i];
            // ... and copy the rest
            m_vbos[i].n_components = va.m_vbos[i].n_components;
            m_vbos[i].data_vec     = va.m_vbos[i].data_vec;
        }
        // Map attribute incides to buffers
        for (GLuint i = 0; i < static_cast<GLuint>(m_n_vbos); ++i) {
            gl::EnableVertexAttribArray(i);
            #ifdef OLD_STYLE_BINDING
                gl::BindBuffer(gl::ARRAY_BUFFER, m_vbos[i].handle);
                gl::VertexAttribPointer(i, m_vbos[i].n_components, gl::FLOAT, GL_FALSE, 0, nullptr);
            #else
                gl::BindVertexBuffer(i, m_vbos[i].handle, 0, m_vbos[i].n_components * sizeof(GLfloat));
                gl::VertexAttribFormat(i, m_vbos[i].n_components, gl::FLOAT, GL_FALSE, 0);
                gl::VertexAttribBinding(i, i);
            #endif
        }
        if (va.isIndexed()) {
            // Generate a new handle...
            gl::GenBuffers(1, &m_ibo->handle);
            // ... and copy the data
            m_ibo->data_vec = va.m_ibo->data_vec;
        }
        if (va.m_is_buffered) { buffer(); }
    }
    return *this;
}

GLVertArray::GLVertArray(GLVertArray&& va): m_handle{va.m_handle}, m_n_vbos{va.m_n_vbos},
                                            m_vbos{va.m_vbos}, m_ibo{va.m_ibo},
                                            m_num_vert{va.m_num_vert},
                                            m_is_buffered{va.m_is_buffered} {
    // Mark as moved
    va.m_handle = 0;
}

GLVertArray& GLVertArray::operator=(GLVertArray&& va) {
    assert(this != &va);
    memcpy(this, &va, sizeof(*this));
    // Mark as moved
    va.m_handle = 0;
    return *this;
}

GLVertArray::~GLVertArray() {
    // Check if it was moved
    if (m_handle) {
        if (isIndexed()) {
            gl::DeleteBuffers(1, &m_ibo->handle);
            delete m_ibo;
        }
        GLuint buffer_handles[MAX_N_VBOS];
        for (auto i = 0; i < m_n_vbos; ++i) {
            buffer_handles[i] = m_vbos[i].handle;
        }
        delete[] m_vbos;
        gl::DeleteBuffers(m_n_vbos, buffer_handles);
        gl::DeleteVertexArrays(1, &m_handle);
    }
}

GLuint GLVertArray::id() const {
    return m_handle;
}

bool GLVertArray::isIndexed() const {
    return (nullptr != m_ibo);
}

void GLVertArray::loadAttrData(const GLuint attr_id, const std::vector<GLfloat>& data_vec) {
    loadAttrData(attr_id, data_vec.size(), data_vec.data());
}

void GLVertArray::loadAttrData(const GLuint attr_id, const size_t n_elems,
                               const GLfloat* const data) {
    for (auto i = 0; i < n_elems; ++i) {
        m_vbos[attr_id].data_vec.push_back(data[i]);
    }
    if (!isIndexed() && 0 == attr_id) {
        // The size of the first buffer determines number of vertices
        m_num_vert = static_cast<GLsizei>(m_vbos[0].data_vec.size() / 3);
    }
    m_is_buffered = false;
}

void GLVertArray::loadIndexData(const std::vector<GLuint>& data_vec, const GLuint offset) {
    loadIndexData(data_vec.size(), data_vec.data(), offset);
}

void GLVertArray::loadIndexData(const size_t n_elems, const GLuint* const data,
                                const GLuint offset) {
    if (!isIndexed()) {
        m_ibo = new IndexBuffer;
        gl::GenBuffers(1, &m_ibo->handle);
        m_ibo->min_idx = UINT_MAX;
        m_ibo->max_idx = 0;
    }
    for (auto i = 0; i < n_elems; ++i) {
        const GLuint idx{offset + data[i]};
        m_ibo->min_idx = min(m_ibo->min_idx, idx);
        m_ibo->max_idx = max(m_ibo->max_idx, idx);
        m_ibo->data_vec.push_back(idx);
    }
    m_num_vert = static_cast<GLsizei>(m_ibo->data_vec.size());
    m_is_buffered = false;
}

void GLVertArray::buffer() {
    for (auto i = 0; i < m_n_vbos; ++i) {
        gl::BindBuffer(gl::ARRAY_BUFFER, m_vbos[i].handle);
        const auto byte_sz = m_vbos[i].data_vec.size() * sizeof(GLfloat);
        gl::BufferData(gl::ARRAY_BUFFER, byte_sz, m_vbos[i].data_vec.data(), gl::STATIC_DRAW);
    }
    if (isIndexed()) {
        gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, m_ibo->handle);
        const auto byte_sz = m_ibo->data_vec.size() * sizeof(GLuint);
        gl::BufferData(gl::ELEMENT_ARRAY_BUFFER, byte_sz, m_ibo->data_vec.data(), gl::STATIC_DRAW);
    }
    m_is_buffered = true;
}


void GLVertArray::draw() const {
    assert(m_is_buffered);
    gl::BindVertexArray(m_handle);
    if (isIndexed()) {
        gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, m_ibo->handle);
        gl::DrawRangeElements(gl::TRIANGLES, m_ibo->min_idx, m_ibo->max_idx, m_num_vert,
                              gl::UNSIGNED_INT, nullptr);
    } else {
        gl::DrawArrays(gl::TRIANGLES, 0, m_num_vert);
    }
}
