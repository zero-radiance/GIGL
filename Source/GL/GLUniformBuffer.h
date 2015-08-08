#pragma once

#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

/* Implements OpenGL uniform buffers of non-std layout */
class GLUniformBuffer {
public:
    GLUniformBuffer() = delete;
    RULE_OF_FIVE(GLUniformBuffer);
    // Constructs an UBO for a (named) block with specified storage members
    explicit GLUniformBuffer(const char* const block_name, const GLuint n_members,
                             const char** const member_names, const GLuint prog_handle);
    // Computes stride (step size in bytes) of array of structs
    GLint calcStructStride(const GLint init_struct_cnt) const;
    // Loads single struct into array of structs
    void loadStructToArray(const GLint index, const GLint stride, const GLuint* const elem_byte_sz,
                           const void* const data);
    // Make a local copy of data, and then copy it to GPU
    // Requires size in bytes specified for each element (member)
    void buffer(const GLuint* const elem_byte_sz, const void* const data);
    // Make a local copy of data, and then copy it to GPU
    // Copies the whole block of data, ignoring offsets. Useful for std140 layout
    void buffer(const GLuint data_byte_sz, const void* const data);
    // Copies preloaded data to GPU
    void buffer();
    // Bind uniform buffer object to to uniform buffer binding point
    void bind(const GLuint bind_idx) const;
private:
    // Performs buffer destruction
    void destroy();
    // Private data members
    GLuint	 m_handle;		    // OpenGL handle
    GLuint   m_n_members;       // Number of buffer data members
    GLint	 m_block_sz;        // Uniform block size in bytes
    GLubyte* m_block_buffer;    // Block buffer contents
    GLint*	 m_offsets;         // Offset to each data member in bytes
    bool     m_is_buffered;     // Flag indicating whether data is buffered on GPU
};
