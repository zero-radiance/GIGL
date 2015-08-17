#include "GLVertArray.h"
#include <cassert>
#include <OpenGL\gl_core_4_4.hpp>
#include "..\Common\Constants.h"

GLVertArray::GLVertArray(const GLuint n_attr, const GLsizei* const component_counts):
                         m_n_vbos{n_attr}, m_vbos{new VertBuffer[n_attr]}, m_is_buffered{false} {
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
    if (va.m_is_buffered) { buffer(); }
}

GLVertArray& GLVertArray::operator=(const GLVertArray& va) {
    if (this != &va) {
        // Destroy the old array
        // Technically, it may be possible to reuse it, but
        // destroying it and creating a new one is simpler and faster
        destroy();
        // Now copy the data
        m_n_vbos      = va.m_n_vbos;
        m_vbos        = new VertBuffer[m_n_vbos];
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
        if (va.m_is_buffered) { buffer(); }
    }
    return *this;
}

GLVertArray::GLVertArray(GLVertArray&& va): m_handle{va.m_handle}, m_n_vbos{va.m_n_vbos},
                                            m_vbos{va.m_vbos}, m_is_buffered{va.m_is_buffered} {
    // Mark as moved
    va.m_handle = 0;
}

GLVertArray& GLVertArray::operator=(GLVertArray&& va) {
    assert(this != &va);
    // Destroy the old array
    // Technically, it may be possible to reuse it, but
    // destroying it and creating a new one is simpler and faster
    destroy();
    // Now copy the data
    memcpy(this, &va, sizeof(*this));
    // Mark as moved
    va.m_handle = 0;
    return *this;
}

GLVertArray::~GLVertArray() {
    // Check if it was moved
    if (m_handle) {
        destroy();
    }
}

void GLVertArray::destroy() {
    GLuint buffer_handles[MAX_N_VBOS];
    for (auto i = 0; i < m_n_vbos; ++i) {
        buffer_handles[i] = m_vbos[i].handle;
    }
    delete[] m_vbos;
    gl::DeleteBuffers(m_n_vbos, buffer_handles);
    gl::DeleteVertexArrays(1, &m_handle);
}

GLuint GLVertArray::id() const {
    assert(m_is_buffered);
    return m_handle;
}

void GLVertArray::loadData(const GLuint attr_id, const std::vector<GLfloat>& data_vec) {
    loadData(attr_id, data_vec.size(), data_vec.data());
}

void GLVertArray::loadData(const GLuint attr_id, const size_t n_elems,
                               const GLfloat* const data) {
    m_vbos[attr_id].data_vec.reserve(m_vbos[attr_id].data_vec.size() + n_elems);
    m_vbos[attr_id].data_vec.insert(m_vbos[attr_id].data_vec.end(), data, data + n_elems);
    m_is_buffered = false;
}

void GLVertArray::buffer() {
    for (auto i = 0; i < m_n_vbos; ++i) {
        gl::BindBuffer(gl::ARRAY_BUFFER, m_vbos[i].handle);
        const auto byte_sz = m_vbos[i].data_vec.size() * sizeof(GLfloat);
        gl::BufferData(gl::ARRAY_BUFFER, byte_sz, m_vbos[i].data_vec.data(), gl::STATIC_DRAW);
    }
    m_is_buffered = true;
}


void GLVertArray::draw(const GLenum mode) const {
    assert(m_is_buffered);
    gl::BindVertexArray(m_handle);
    // The size of the first buffer determines the number of vertices
    const auto n_vert = static_cast<GLsizei>(m_vbos[0].data_vec.size() / 3);
    gl::DrawArrays(mode, 0, n_vert);
}
