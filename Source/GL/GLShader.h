#pragma once

#include <vector>
#include <GLM\fwd.hpp>
#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

/* Class representing GLSL program */
class GLSLProgram {
public:
    GLSLProgram();
    RULE_OF_FIVE(GLSLProgram);
    // Loads shader from disk
    void loadShader(const char* const file_name);
    // Links GLSL program
    void link();
    // Activates GSLS program
    void use() const;
    // Returns program's unique handle
    GLuint id() const;
    // Returns true if program status is OK
    bool isValid() const;
    // Returns location assigned to a uniform variable
    GLint getUniformLocation(const char* const name) const;
    // Sets named uniform variable to specified value
    template <typename T>
    void setUniformValue(const char* const name, const T& v) const;
    // Sets uniform variable to specified value using its location
    static void setUniformValue(const GLint loc, const GLboolean& v);
    static void setUniformValue(const GLint loc, const GLint&     v);
    static void setUniformValue(const GLint loc, const GLuint&    v);
    static void setUniformValue(const GLint loc, const GLfloat&   v);
    static void setUniformValue(const GLint loc, const glm::vec3& v);
    static void setUniformValue(const GLint loc, const glm::vec4& v);
    static void setUniformValue(const GLint loc, const glm::mat3& m);
    static void setUniformValue(const GLint loc, const glm::mat4& m);
private:
    /* Class representing GLSL shader */
    class GLSLShader {
    public:
        GLSLShader() = delete;
        RULE_OF_FIVE(GLSLShader);
        // Creates a shader from a file on disk
        explicit GLSLShader(const char* const file_name);
        // Returns shader's unique handle
        GLuint id() const;
        // Returns true if shader status is OK
        bool isValid() const;
    private:
        // Reads shader from file on disk; also returns length of shader code
        const char* loadShaderFromFile(const char* const file_name, GLint& code_len);
        // Performs shader compilation
        void compile();
        // Private data members
        GLuint m_handle;                    // Unique OpenGL handle
        bool   m_is_ok;                     // Status
    };
    // Attaches (valid) shader to the shader program
    void attachShader(const GLSLShader& sh);
    // Private data members
    GLuint			        m_handle;       // Unique OpenGL handle
    bool					m_is_ok;        // Status
    bool                    m_is_linked;    // Program linking status
    std::vector<GLSLShader> m_shaders;      // Attached shaders
};
