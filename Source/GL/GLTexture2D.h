#pragma once
#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

/* Implements 2D OpenGL texture of the specified Sized Internal Format */
template <GLenum SIF>
class GLTexture2D {
public:
    GLTexture2D() = delete;
    RULE_OF_FIVE_NO_COPY(GLTexture2D);
    // Creates a texture of specified width and height bound to the specified tex. unit
    // Can use MIP-maps and linear texture filering if desired
    explicit GLTexture2D(const GLenum tex_unit, const GLsizei width, const GLsizei height,
                         const bool use_mipmaps, const bool use_lin_filter);
    // Returns texture's OpenGL handle
    GLuint id() const;
private:
    GLuint m_handle;   // OpenGL handle
};

using GLTex2D_3x8   = GLTexture2D<gl::RGB8>;
using GLTex2D_1x32F = GLTexture2D<gl::R32F>;
using GLTex2D_2x32F = GLTexture2D<gl::RG32F>;
using GLTex2D_3x32F = GLTexture2D<gl::RGB32F>;
using GLTex2D_4x32F = GLTexture2D<gl::RGBA32F>;
