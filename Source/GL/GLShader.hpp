#pragma once

#include "GLShader.h"

template <typename T>
void GLSLProgram::setUniformValue(const char* const name, const T& v) const {
    const GLint loc{getUniformLocation(name)};
    setUniformValue(loc, v);
}
