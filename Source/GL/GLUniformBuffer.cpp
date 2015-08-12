#include "GLUniformBuffer.h"
#include <cassert>
#include <OpenGL\gl_core_4_4.hpp>
#include "..\Common\Utility.hpp"

GLUniformBuffer::GLUniformBuffer(const char* const block_name, const GLuint n_members,
                                 const char** const member_names, const GLuint prog_handle):
                                 m_n_members{n_members}, m_is_buffered{false} {
    // Generate buffer
    gl::GenBuffers(1, &m_handle);
    // Query for block index
    const GLuint block_id{gl::GetUniformBlockIndex(prog_handle, block_name)};
    // Query for block size
    gl::GetActiveUniformBlockiv(prog_handle, block_id, gl::UNIFORM_BLOCK_DATA_SIZE,
                                reinterpret_cast<GLint*>(&m_block_sz));
    // Allocate buffer on GPU
    gl::BindBuffer(gl::UNIFORM_BUFFER, m_handle);
    gl::BufferData(gl::UNIFORM_BUFFER, m_block_sz, nullptr, gl::DYNAMIC_DRAW);
    // Allocate buffer on CPU
    m_block_buffer = new GLubyte[m_block_sz];
    if (member_names) {
        // Query for member indices
        GLuint* indices = new GLuint[m_n_members];
        gl::GetUniformIndices(prog_handle, m_n_members, member_names, indices);
        // Query for member offsets
        m_offsets = new GLint[m_n_members];
        gl::GetActiveUniformsiv(prog_handle, m_n_members, indices, gl::UNIFORM_OFFSET, m_offsets);
        // Free memory
        delete[] indices;
    }
}

GLUniformBuffer::GLUniformBuffer(const GLUniformBuffer& ubo): m_n_members{ubo.m_n_members},
                 m_block_sz{ubo.m_block_sz}, m_block_buffer{new GLubyte[m_block_sz]},
                 m_offsets{new GLint[m_n_members]}, m_is_buffered{ubo.m_is_buffered} {
    memcpy(m_block_buffer, ubo.m_block_buffer, m_block_sz * sizeof(GLubyte));
    memcpy(m_offsets, ubo.m_offsets, m_n_members * sizeof(GLint));
    // Create a new buffer on GPU
    gl::GenBuffers(1, &m_handle);
    gl::BindBuffer(gl::UNIFORM_BUFFER, m_handle);
    gl::BufferData(gl::UNIFORM_BUFFER, m_block_sz, m_is_buffered ? m_block_buffer : nullptr,
                   gl::DYNAMIC_DRAW);
}

GLUniformBuffer& GLUniformBuffer::operator=(const GLUniformBuffer& ubo) {
    if (this != &ubo) {
        // Destroy the old buffer
        // Technically, it may be possible to reuse it, but
        // destroying it and creating a new one is simpler and faster
        destroy();
        // Now copy the data
        m_n_members    = ubo.m_n_members;
        m_block_sz     = ubo.m_block_sz;
        m_block_buffer = new GLubyte[m_block_sz];
        m_offsets      = new GLint[m_n_members];
        m_is_buffered  = ubo.m_is_buffered;
        memcpy(m_block_buffer, ubo.m_block_buffer, m_block_sz * sizeof(GLubyte));
        memcpy(m_offsets, ubo.m_offsets, m_n_members * sizeof(GLint));
        // Create a new buffer on GPU
        gl::GenBuffers(1, &m_handle);
        gl::BindBuffer(gl::UNIFORM_BUFFER, m_handle);
        gl::BufferData(gl::UNIFORM_BUFFER, m_block_sz, m_is_buffered ? m_block_buffer : nullptr,
                       gl::DYNAMIC_DRAW);
    }
    return *this;
}

GLUniformBuffer::GLUniformBuffer(GLUniformBuffer&& ubo): m_handle{ubo.m_handle},
                 m_n_members{ubo.m_n_members}, m_block_sz{ubo.m_block_sz},
                 m_block_buffer{ubo.m_block_buffer}, m_offsets{ubo.m_offsets},
                 m_is_buffered{ubo.m_is_buffered} {
    // Mark as moved
    ubo.m_handle = 0;
}

GLUniformBuffer& GLUniformBuffer::operator=(GLUniformBuffer&& ubo) {
    assert(this != &ubo);
    // Destroy the old buffer
    // Technically, it may be possible to reuse it, but
    // destroying it and creating a new one is simpler and faster
    destroy();
    // Now copy the data
    memcpy(this, &ubo, sizeof(*this));
    // Mark as moved
    ubo.m_handle = 0;
    return *this;
}

GLUniformBuffer::~GLUniformBuffer() {
    // Check if it was moved
    if (m_handle) {
        destroy();
    }
}

void GLUniformBuffer::destroy() {
    delete[] m_block_buffer;
    delete[] m_offsets;
    gl::DeleteBuffers(1, &m_handle);
}

GLint GLUniformBuffer::calcStructStride(const GLint init_struct_cnt) const {
    return m_block_sz / init_struct_cnt;
}

void GLUniformBuffer::loadStructToArray(const GLint index, const GLint stride,
                                        const GLuint* const elem_byte_sz,
                                        const void* const data) {
    const auto byte_data = reinterpret_cast<const GLubyte* const>(data);
    GLubyte* const buffer_pos{m_block_buffer + index * stride};
    for (GLuint i = 0, data_offset = 0; i < m_n_members; data_offset += elem_byte_sz[i], i++) {
        memcpy(buffer_pos + m_offsets[i], byte_data + data_offset, elem_byte_sz[i]);
    }
    m_is_buffered = false;
}

void GLUniformBuffer::buffer(const GLuint* const elem_byte_sz, const void* const data) {
    const auto byte_data = reinterpret_cast<const GLubyte* const>(data);
    for (GLuint i = 0, data_offset = 0; i < m_n_members; data_offset += elem_byte_sz[i], i++) {
        memcpy(m_block_buffer + m_offsets[i], byte_data + data_offset, elem_byte_sz[i]);
    }
    buffer();
}

void GLUniformBuffer::buffer(const GLuint data_byte_sz, const void* const data) {
    assert(data_byte_sz <= m_block_sz);
    memcpy(m_block_buffer, data, data_byte_sz);
    buffer();
}

void GLUniformBuffer::buffer() {
    gl::BindBuffer(gl::UNIFORM_BUFFER, m_handle);
    gl::BufferSubData(gl::UNIFORM_BUFFER, 0, m_block_sz, m_block_buffer);
    m_is_buffered = true;
}

void GLUniformBuffer::bind(const GLuint bind_idx) const {
    gl::BindBufferBase(gl::UNIFORM_BUFFER, bind_idx, m_handle);
}
