#pragma once

#include <vector>
#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

class GLVertArray;

/* OpenGL element (index) buffer */
class GLElementBuffer {
public:
    GLElementBuffer();
    RULE_OF_FIVE(GLElementBuffer);
    // Copies preloaded data to GPU
    void buffer();
    // Load indexing information from std::vector
    void loadData(const std::vector<GLuint>& data_vec, const GLuint offset);
    // Load indexing information from an array
    void loadData(const size_t n_elems, const GLuint* const data, const GLuint offset);
    // Draws indexed vertex array
    void draw(const GLVertArray& va) const;
private:
    GLuint m_handle;                    // Unique OpenGL handle
    GLuint m_min_idx, m_max_idx;        // Minimal and maximal indices
    bool   m_is_buffered;               // Flag indicating whether data is buffered on GPU
    std::vector<GLuint> m_data_vec;     // Storage
};
