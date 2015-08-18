#pragma once

#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

/* Implements OpenGL texture buffers */
class GLTextureBuffer {
public:
    GLTextureBuffer() = delete;
    RULE_OF_FIVE_NO_COPY(GLTextureBuffer);
    // Creates a texture buffer of the specified size and the specified internal format
    explicit GLTextureBuffer(const GLsizeiptr byte_sz, const GLenum tex_unit,
                             const GLenum tex_intern_fmt);
    // Copies the data of the specified size to the GPU
    void bufferData(const GLsizeiptr byte_sz, const void* const data);
private:
    GLuint     m_handle;        // OpenGL buffer handle
    GLuint     m_tex_handle;    // OpenGL texture handle
    GLsizeiptr m_byte_sz;       // Buffer size in bytes
};
