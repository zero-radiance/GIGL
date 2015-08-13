#include "GLTexture2D.h"
#include <GLM\detail\func_common.hpp>

// Computes the number of MIP-map levels given the texture dimensions
static inline GLsizei computeMipLvlCnt(const GLsizei width, const GLsizei height) {
    return static_cast<GLsizei>(floor(log2(glm::max(width, height))) + 1);
}

template <GLenum SIF>
GLTexture2D<SIF>::GLTexture2D(const GLenum tex_unit, const GLsizei width, const GLsizei height,
                               const bool use_mipmaps, const bool use_lin_filter) {
    // Aquire a texture handle
    gl::ActiveTexture(gl::TEXTURE0 + tex_unit);
    gl::GenTextures(1, &m_handle);
    gl::BindTexture(gl::TEXTURE_2D, m_handle);
    // Compute the number of MIP-map levels
    const GLsizei levels{use_mipmaps ? computeMipLvlCnt(width, height) : 1};
    // Allocate storage
    gl::TexStorage2D(gl::TEXTURE_2D, levels, SIF, width, height);
    // Set texture filering
    const GLint mag_filter{use_lin_filter ? gl::LINEAR : gl::NEAREST};
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, mag_filter);
    const GLint min_filter{[=](){
        if (use_mipmaps) {
            return use_lin_filter ? gl::NEAREST_MIPMAP_NEAREST : gl::NEAREST_MIPMAP_LINEAR;
        } else {
            return use_lin_filter ? gl::LINEAR : gl::NEAREST;
        }
    }()};
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, min_filter);
    // Use border-clamping for both dimensions
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_BORDER);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_BORDER);
}

template <GLenum SIF>
GLTexture2D<SIF>::GLTexture2D(GLTexture2D&& tex): m_handle{tex.m_handle} {
    // Mark as moved
    tex.m_handle = 0;
}

template <GLenum SIF>
GLTexture2D<SIF>& GLTexture2D<SIF>::operator=(GLTexture2D&& tex) {
    assert(this != &tex);
    // Delete the old texture
    gl::DeleteTextures(1, &m_handle);
    // Now copy the data
    memcpy(this, &tex, sizeof(*this));
    // Mark as moved
    tex.m_handle = 0;
    return *this;
}

template <GLenum SIF>
GLTexture2D<SIF>::~GLTexture2D() {
    // Check if it was moved
    if (m_handle) {
        gl::DeleteTextures(1, &m_handle);
    }
}

template <GLenum SIF>
GLuint GLTexture2D<SIF>::id() const {
    return m_handle;
}
