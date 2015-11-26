#pragma once

#include <vector>
#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

class GLElementBuffer;

/* Class representing OpenGL vertex array */
class GLVertArray {
public:
    GLVertArray() = delete;
    RULE_OF_FIVE(GLVertArray);
    // Constructs a vertex array with specified attributes
    explicit GLVertArray(const GLsizei n_attr, const GLsizei* const component_counts);
    // Returns vertex array's OpenGL handle
    GLuint id() const;
    // Loads data for specified attribute from std::vector
    void loadData(const GLuint attr_id, const std::vector<GLfloat>& data_vec);
    // Loads data for specified attribute from an array
    void loadData(const GLuint attr_id, const size_t n_elems, const GLfloat* const data);
    // Copies preloaded data to GPU
    void buffer();
    // Draws vertex array in specified a mode (such as gl::TRIANGLES)
    void draw(const GLenum mode) const;
private:
    /* Vertex Buffer Object */
    struct VertBuffer {
        VertBuffer() = default;
        RULE_OF_ZERO(VertBuffer);
        GLuint  handle;                 // OpenGL handle
        GLsizei n_components;           // Number of FP components per element
        std::vector<GLfloat> data_vec;  // Storage
    };
    // Performs array destruction
    void destroy();
    // Private data members
    GLuint        m_handle;             // OpenGL handle
    GLsizei       m_n_vbos;             // Number of Vertex Buffer Objects (VBOs)
    VertBuffer*   m_vbos;               // Array of Vertex Buffer Objects (VBOs)
    bool          m_is_buffered;        // Flag indicating whether data is buffered on GPU
};
