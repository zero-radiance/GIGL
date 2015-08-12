#include "GLUniformManager.h"
#include "GLShader.h"

template <int N>
GLUniformManager<N>::GLUniformManager(const GLSLProgram& sp,
                                      std::initializer_list<const char*> names) {
    setManagedUniforms(sp, names);
}

template <int N>
void GLUniformManager<N>::setManagedUniforms(const GLSLProgram& sp,
                                             std::initializer_list<const char*> names) {
    #if __cplusplus <= 201103L
        // No constexpr support
        assert(names.size() == N);
    #else
        static_assert(names.size() == N, "Mismatch between the number of uniforms N and "
                                        "the number of function arguments.");
    #endif
    // Query for locations
    int i = 0;
    for (const auto name : names) {
        m_uni_locs[i++] = sp.getUniformLocation(name);
    }
}

template <int N>
template <typename... Args>
void GLUniformManager<N>::setUniformValues(const Args&... vals) const {
    setUniforms<0>(vals...);
}

template <int N>
template <int I, typename T, typename... Args>
void GLUniformManager<N>::setUniforms(const T& first, const Args&... vals) const {
    static_assert(I <= N - 1, "Invalid number of function arguments.");
    GLSLProgram::setUniformValue(m_uni_locs[I], first);
    setUniforms<I + 1>(vals...);
}

template <int N>
template <int I, typename T>
void GLUniformManager<N>::setUniforms(const T& last) const {
    static_assert(I == N - 1, "Invalid number of function arguments.");
    GLSLProgram::setUniformValue(m_uni_locs[I], last);
}
