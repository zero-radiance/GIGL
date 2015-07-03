#include "GLUniformManager.h"
#include "GLShader.h"

template <int N>
GLUniformManager<N>::GLUniformManager(const GLSLProgram& sp,
                                      std::initializer_list<const char*> args) {
    #if __cplusplus <= 201103L
        // No constexpr support
        assert(args.size() == N);
    #else
        static_assert(args.size() == N, "Mismatch between the number of uniforms N and "
                                        "the number of function arguments.");
    #endif
    // Query for locations
    int i = 0;
    for (const auto arg : args) {
        m_uni_locs[i++] = sp.getUniformLocation(arg);
    }
}

template <int N>
template <typename... Args>
void GLUniformManager<N>::setUniformValues(const Args&... args) const {
    setUniforms<0>(args...);
}

template <int N>
template <int I, typename T, typename... Args>
void GLUniformManager<N>::setUniforms(const T& first, const Args&... args) const {
    static_assert(I <= N - 1, "Invalid number of function arguments.");
    GLSLProgram::setUniformValue(m_uni_locs[I], first);
    setUniforms<I + 1>(args...);
}

template <int N>
template <int I, typename T>
void GLUniformManager<N>::setUniforms(const T& last) const {
    static_assert(I == N - 1, "Invalid number of function arguments.");
    GLSLProgram::setUniformValue(m_uni_locs[I], last);
}
