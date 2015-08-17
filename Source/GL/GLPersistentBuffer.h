#pragma once

#include <OpenGL\gl_core_4_4.hpp>
#include "..\Common\Definitions.h"

/* Implements persistent OpenGL buffer with EXPLICIT layout, e.g.
   (std140) uniform buffers or (std430) shader storage buffers */
template <GLenum T>
class GLPersistentBuffer {
public:
    GLPersistentBuffer() = delete;
    RULE_OF_FIVE(GLPersistentBuffer);
    // Creates a persistent buffer of specified size
    explicit GLPersistentBuffer(const size_t byte_sz);
    // Returns the pointer to the beginning of the buffer
    void* data();
    // Returns the pointer to the beginning of the buffer (read-only)
    const void* data() const;
    // Binds buffer to buffer binding point
    void bind(const GLuint bind_idx) const;
private:
    // Performs buffer destruction
    void destroy();
    // Private data members
    void*  m_buffer;     // Pointer used to write to the buffer
    size_t m_byte_sz;    // Buffer size in bytes
    GLuint m_handle;     // OpenGL handle
};

using GLPUB140 = GLPersistentBuffer<gl::UNIFORM_BUFFER>;
using GLPSB430 = GLPersistentBuffer<gl::SHADER_STORAGE_BUFFER>;
