#pragma once

#include <vector>
#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

/* Class representing OpenGL vertex array */
class GLVertArray {
public:
    GLVertArray() = delete;
    RULE_OF_FIVE(GLVertArray);
    // Constructs a vertex array with specified attributes
    explicit GLVertArray(const GLuint n_attr, const GLsizei* const component_counts);
    // Returns vertex array's unique handle
    GLuint id() const;
    // Returns true if indexed drawing is supported
    bool isIndexed() const;
    // Loads data for specified attribute from std::vector
    void loadAttrData(const GLuint attr_id, const std::vector<GLfloat>& data_vec);
    // Loads data for specified attribute from an array
    void loadAttrData(const GLuint attr_id, const size_t n_elems, const GLfloat* const data);
    // Load indexing information for indexed drawing from std::vector
    void loadIndexData(const std::vector<GLuint>& data_vec, const GLuint offset);
    // Load indexing information for indexed drawing from an array
    void loadIndexData(const size_t n_elems, const GLuint* const data, const GLuint offset);
    // Copies preloaded data to GPU
    void buffer();
    // Draws vertex array
    void draw() const;
private:
    /* Vertex Buffer Object */
    struct VertBuffer {
        VertBuffer() = default;
        RULE_OF_ZERO(VertBuffer);
        GLuint	handle;                 // Unique OpenGL handle
        GLsizei n_components;           // Number of FP components per element
        std::vector<GLfloat> data_vec;  // Storage
    };
    /* Index (Element) Buffer Object */
    struct IndexBuffer {
        IndexBuffer() = default;
        RULE_OF_ZERO(IndexBuffer);
        GLuint handle;                  // Unique OpenGL handle
        GLuint min_idx, max_idx;        // Minimal and maximal contained values
        std::vector<GLuint> data_vec;   // Storage
    };
    // Performs array destruction
    void destroy();
    // Private data members
    GLuint		  m_handle;             // Unique OpenGL handle
    GLsizei       m_n_vbos;             // Number of Vertex Buffer Objects (VBOs)
    VertBuffer*   m_vbos;               // Array of Vertex Buffer Objects (VBOs)
    IndexBuffer*  m_ibo;                // Index (Element) Buffer Object, if available
    GLsizei		  m_num_vert;           // Number of vertices within the array
    bool          m_is_buffered;        // Flag indicating whether data is buffered on GPU
};
