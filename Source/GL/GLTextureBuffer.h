#pragma once

#include <OpenGL\gl_core_4_4.hpp>
#include "..\Common\Definitions.h"

/* Implements OpenGL texture buffers */
class GLTextureBuffer {
public:
    GLTextureBuffer() = delete;
    RULE_OF_FIVE_NO_COPY(GLTextureBuffer);
    // Creates a texture buffer using specified internal format,
    // with data of specified size
    explicit GLTextureBuffer(const GLenum tex_unit, const GLenum tex_intern_fmt,
                             const GLsizeiptr byte_sz, const void* const data);
private:
    GLuint m_handle;        // Unique OpenGL buffer handle
    GLuint m_tex_handle;    // Unique OpenGL texture handle
};
