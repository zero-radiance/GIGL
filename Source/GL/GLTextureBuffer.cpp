#include "GLTextureBuffer.h"

GLTextureBuffer::GLTextureBuffer(const GLenum tex_unit,const GLenum tex_intern_fmt,
                                 const GLsizeiptr byte_sz, const void* const data) {
    // Generate buffer
    gl::GenBuffers(1, &m_handle);
    gl::BindBuffer(gl::TEXTURE_BUFFER, m_handle);
    gl::BufferData(gl::TEXTURE_BUFFER, byte_sz, data, gl::STATIC_DRAW);
    // Create texture
    gl::ActiveTexture(gl::TEXTURE0 + tex_unit);
    gl::GenTextures(1, &m_tex_handle);
    gl::BindTexture(gl::TEXTURE_BUFFER, m_tex_handle);
    gl::TexBuffer(gl::TEXTURE_BUFFER, tex_intern_fmt, m_handle);
}

GLTextureBuffer::GLTextureBuffer(GLTextureBuffer&& tbo): m_handle{tbo.m_handle},
                                                         m_tex_handle{tbo.m_tex_handle} {
    // Mark as moved
    tbo.m_handle = 0;
}

GLTextureBuffer& GLTextureBuffer::operator=(GLTextureBuffer&& tbo) {
    memcpy(this, &tbo, sizeof(*this));
    // Mark as moved
    tbo.m_handle = 0;
    return *this;
}

GLTextureBuffer::~GLTextureBuffer() {
    // Check if it was moved
    if (m_handle) {
        gl::DeleteTextures(1, &m_tex_handle);
        gl::DeleteBuffers(1, &m_handle);
    }
}
