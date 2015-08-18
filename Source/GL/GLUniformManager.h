#pragma once

#include <array>
#include <initializer_list>

class GLSLProgram;

/* OpenGL uniform manager class for N uniforms */
template <int N>
class GLUniformManager {
public:
    GLUniformManager() = default;
    RULE_OF_ZERO(GLUniformManager);
    // Constructor, takes an GLSL program's reference and a list of uniform names as arguments
    GLUniformManager(const GLSLProgram& sp, std::initializer_list<const char*> names);
    // Initializes the list of the uniforms to be managed
    void setManagedUniforms(const GLSLProgram& sp, std::initializer_list<const char*> names);
    // Sets uniform values; use THE SAME order as during initialization!
    template <typename... Args>
    void setUniformValues(const Args&... vals) const;
private:
    // Recursively sets uniform values using m_uni_locs[I]
    template <int I, typename T, typename... Args>
    void setUniforms(const T& first, const Args&... vals) const;
    // Sets the last uniform value using m_uni_locs[N - 1]
    template <int I, typename T>
    void setUniforms(const T& last) const;
    // Private data members
    std::array<GLint, N> m_uni_locs;    // Uniform locations
};
