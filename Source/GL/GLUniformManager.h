#pragma once

#include <array>
#include <initializer_list>

class GLSLProgram;

/* OpenGL Uniform Manager class for N uniforms */
template <int N>
class GLUniformManager {
public:
    GLUniformManager() = delete;
    RULE_OF_ZERO(GLUniformManager);
    // Constructor, takes GLSL program handle and list of uniform names as arguments
    GLUniformManager(const GLSLProgram& sp, std::initializer_list<const char*> args);
    // Sets uniform values; use THE SAME order and count as in the constructor!
    template <typename... Args>
    void setUniformValues(const Args&... args) const;
private:
    // Recursively sets uniform values using m_uni_locs[I]
    template <int I, typename T, typename... Args>
    void setUniforms(const T& first, const Args&... args) const;
    // Sets the last uniform value using m_uni_locs[N - 1]
    template <int I, typename T>
    void setUniforms(const T& last) const;
    // Private data members
    std::array<GLint, N> m_uni_locs;    // Uniform locations
};
