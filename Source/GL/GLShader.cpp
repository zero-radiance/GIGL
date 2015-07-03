#include "GLShader.h"
#include <map>
#include <GLM\mat3x3.hpp>
#include <GLM\mat4x4.hpp>
#include <OpenGL\gl_core_4_4.hpp>
#include "..\Common\Constants.h"
#include "..\Common\Utility.hpp"

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

// Null-terminated C-string comparison functor
struct CStrCmp {
    bool operator()(const char* const a, const char* const b) {
        return strcmp(a, b) < 0;
    }
};

// GLSL file extension map
CONSTEXPR std::map<const char*, GLenum, CStrCmp> file_exts = {{".vs",   gl::VERTEX_SHADER},
                                                              {".vert", gl::VERTEX_SHADER},
                                                              {".gs",   gl::GEOMETRY_SHADER},
                                                              {".geom", gl::GEOMETRY_SHADER},
                                                              {".tcs",  gl::TESS_CONTROL_SHADER},
                                                              {".tes",  gl::TESS_EVALUATION_SHADER},
                                                              {".fs",   gl::FRAGMENT_SHADER},
                                                              {".frag", gl::FRAGMENT_SHADER},
                                                              {".cs",   gl::COMPUTE_SHADER}};

// Returns file extension with a dot prefix
static inline const char* getDotExt(const char* const file_name) {
    return strchr(file_name, '.');
}

// Prints OpenGL shader [program] info log
static inline void printShaderInfoLog(const GLuint handle, const GLsizei log_len) {
    if (log_len > 0) {
        char* const log{new char[log_len]};
        gl::GetShaderInfoLog(handle, log_len, nullptr, log);
        printError("Shader program log:\n%s", log);
        delete[] log;
        TERMINATE();
    }
}

GLSLProgram::GLSLProgram(): m_handle{gl::CreateProgram()}, m_is_ok{0 != m_handle},
                            m_is_linked{false} {
    if (!m_is_ok) {
        printError("Error while creating shader program.");
        TERMINATE();
    }
    // Reserve memory to avoid shader copies
    m_shaders.reserve(8);
}

GLSLProgram::GLSLProgram(const GLSLProgram& sp): m_handle{gl::CreateProgram()},
                                                 m_is_ok{0 != m_handle}, m_is_linked{false},
                                                 m_shaders(sp.m_shaders) {
    assert(sp.isValid());
    if (m_is_ok) {
        // Attach shaders
        for (const auto& sh : m_shaders) { attachShader(sh); }
        // Link if necessary
        if (sp.m_is_linked) { link(); }
    }
}

GLSLProgram& GLSLProgram::operator=(const GLSLProgram& sp) {
    assert(sp.isValid());
    if (this != &sp) {
        m_handle    = gl::CreateProgram();
        m_is_ok     = (0 != m_handle);
        m_is_linked = false;
        m_shaders   = sp.m_shaders;
        if (m_is_ok) {
            // Attach shaders
            for (const auto& sh : m_shaders) { attachShader(sh); }
            // Link if necessary
            if (sp.m_is_linked) { link(); }
        }
    }
    return *this;
}

GLSLProgram::GLSLProgram(GLSLProgram&& sp): m_handle{sp.m_handle}, m_is_ok{sp.m_is_ok},
                                            m_is_linked{sp.m_is_linked}, m_shaders(sp.m_shaders) {
    assert(sp.isValid());
    // Mark as moved
    sp.m_handle = 0;
}

GLSLProgram& GLSLProgram::operator=(GLSLProgram&& sp) {
    assert(sp.isValid());
    assert(this != &sp);
    memcpy(this, &sp, sizeof(*this));
    // Mark as moved
    sp.m_handle = 0;
    return *this;
}

GLSLProgram::~GLSLProgram() {
    // Check if it was moved
    if (m_handle) {
        gl::DeleteProgram(m_handle);
    }
}

GLuint GLSLProgram::id() const {
    return m_handle;
}

bool GLSLProgram::isValid() const {
    return m_is_ok;
}

void GLSLProgram::loadShader(const char* const file_name) {
    if (m_is_ok) {
        m_shaders.emplace_back(file_name);
        attachShader(m_shaders.back());
    } else {
        printError("Attempted to load shader into an invalid shader program.");
        TERMINATE();
    }
}

void GLSLProgram::attachShader(const GLSLShader& sh) {
    if (sh.isValid()) {
        // Attach a valid shader
        gl::AttachShader(m_handle, sh.id());
    } else {
        m_is_ok = false;
        printError("Attempted to attach an invalid shader to a shader program.");
        TERMINATE();
    }
}

void GLSLProgram::link() {
    if (m_is_ok) {
        // Perform linking
        gl::LinkProgram(m_handle);
        // Verify program
        GLint success;
        gl::GetProgramiv(m_handle, gl::LINK_STATUS, &success);
        if (!success) {
            m_is_ok = false;
            printError("Program linking failed.");
            GLsizei log_len;
            gl::GetProgramiv(m_handle, gl::INFO_LOG_LENGTH, &log_len);
            printShaderInfoLog(m_handle, log_len);
            TERMINATE();
        } else {
            m_is_linked = true;
        }
    } else {
        printError("Attempted to link an invalid shader program.");
        TERMINATE();
    }
}

void GLSLProgram::use() const {
    if (m_is_ok) {
        // Install program into pipeline
        gl::UseProgram(m_handle);
    } else {
        printError("Attempted to run an invalid shader program.");
        TERMINATE();
    }
}

GLint GLSLProgram::getUniformLocation(const char* const name) const {
    const GLint loc{gl::GetUniformLocation(m_handle, name)};
    if (-1 == loc) {
        printError("Uniform location of \"%s\" not found!", name);    
    }
    return loc;
}

void GLSLProgram::setUniformValue(const GLint loc, const GLboolean& v) {
    gl::Uniform1i(loc, v);
}

void GLSLProgram::setUniformValue(const GLint loc, const GLint& v) {
    gl::Uniform1i(loc, v);
}

void GLSLProgram::setUniformValue(const GLint loc, const GLuint& v) {
    gl::Uniform1ui(loc, v);
}

void GLSLProgram::setUniformValue(const GLint loc, const GLfloat& v) {
    gl::Uniform1f(loc, v);
}

void GLSLProgram::setUniformValue(const GLint loc, const vec3& v) {
    gl::Uniform3f(loc, v.x, v.y, v.z);
}

void GLSLProgram::setUniformValue(const GLint loc, const vec4& v) {
    gl::Uniform4f(loc, v.x, v.y, v.z, v.w);
}

void GLSLProgram::setUniformValue(const GLint loc, const mat3& m) {
    gl::UniformMatrix3fv(loc, 1, GL_FALSE, &m[0][0]);
}

void GLSLProgram::setUniformValue(const GLint loc, const mat4& m) {
    gl::UniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
}

GLSLProgram::GLSLShader::GLSLShader(const char* const file_name):
                         m_handle{gl::CreateShader(file_exts.find(getDotExt(file_name))->second)},
                         m_is_ok{0 != m_handle} {
    if (m_is_ok) {
        // Reader shader source code from file
        GLint code_len;
        const GLchar* const shader_code{loadShaderFromFile(file_name, code_len)};
        // Assign shader source code
        const GLchar* const code_array[] = {shader_code};
        gl::ShaderSource(m_handle, 1, code_array, &code_len);
        delete[] shader_code;
        // Compile shader
        compile();
    } else {
        printError("Error while creating vertex shader.");
        TERMINATE();
    }
}
GLSLProgram::GLSLShader::GLSLShader(const GLSLShader& sh) {
    assert(sh.isValid());
    *this = sh;
}

GLSLProgram::GLSLShader& GLSLProgram::GLSLShader::operator=(const GLSLShader& sh) {
    assert(sh.isValid());
    if (this != &sh) {
        // Get shader type
        GLint sh_type;
        gl::GetShaderiv(sh.id(), gl::SHADER_TYPE, &sh_type);
        m_handle = gl::CreateShader(sh_type);
        m_is_ok  = (0 != m_handle);
        if (m_is_ok) {
            // Get shader source code
            GLint code_len;
            gl::GetShaderiv(sh.id(), gl::SHADER_SOURCE_LENGTH, &code_len);
            GLchar* const shader_code{new char[code_len]};
            gl::GetShaderSource(sh.id(), code_len, nullptr, shader_code);
            // Assign shader source code
            const GLchar* const code_array[] = {shader_code};
            gl::ShaderSource(m_handle, 1, code_array, &code_len);
            delete[] shader_code;
            // Compile shader
            compile();
        } else {
            printError("Error while creating vertex shader.");
            TERMINATE();
        }
    }
    return *this;
}

GLSLProgram::GLSLShader::GLSLShader(GLSLShader&& sh): m_handle{sh.m_handle}, m_is_ok{sh.m_is_ok} {
    assert(sh.isValid());
    // Mark as moved
    sh.m_handle = 0;
}

GLSLProgram::GLSLShader& GLSLProgram::GLSLShader::operator=(GLSLShader&& sh) {
    assert(sh.isValid());
    assert(this != &sh);
    memcpy(this, &sh, sizeof(*this));
    // Mark as moved
    sh.m_handle = 0;
    return *this;
}

GLSLProgram::GLSLShader::~GLSLShader() {
    // Check if it was moved
    if (m_handle) {
        gl::DeleteShader(m_handle);
    }
}

GLuint GLSLProgram::GLSLShader::id() const {
    return m_handle;
}

bool GLSLProgram::GLSLShader::isValid() const {
    return m_is_ok;
}

const char* GLSLProgram::GLSLShader::loadShaderFromFile(const char* const file_name,
                                                        GLint& code_len) {
    auto file = fopen(file_name, "rb");
    if (!file) {
        m_is_ok = false;
        printError("Shader file %s not found.", file_name);
        TERMINATE();
    }
    // Get file size in bytes
    fseek(file, 0, SEEK_END);
    code_len = ftell(file);
    rewind(file);
    // Read file, add one extra character for null termination
    char* const buffer{new char[code_len + 1]};
    fread(buffer, 1, code_len, file);
    // Null-terminate string
    buffer[code_len++] = '\0';
    // Close file and return shader string
    fclose(file);
    return buffer;
}

void GLSLProgram::GLSLShader::compile() {
    gl::CompileShader(m_handle);
    // Verify shader
    GLint success;
    gl::GetShaderiv(m_handle, gl::COMPILE_STATUS, &success);
    if (!success) {
        m_is_ok = false;
        printError("Shader compilation failed.");
        GLsizei log_len;
        gl::GetShaderiv(m_handle, gl::INFO_LOG_LENGTH, &log_len);
        printShaderInfoLog(m_handle, log_len);
        TERMINATE();
    }
}
