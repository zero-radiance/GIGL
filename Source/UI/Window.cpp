#include "Window.h"
#include <cassert>
#include <OpenGL\gl_core_4_4.hpp>
#include <GLFW\glfw3.h>
#include "..\Common\Constants.h"
#include "..\Common\Utility.hpp"

static inline void errorCallback(const int, const char* const msg) {
    printError(msg);
}

static inline void keyCallback(GLFWwindow* const wnd, const int key, const int,
                               const int action, const int) {
    if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
        glfwSetWindowShouldClose(wnd, GL_TRUE);
    }
}

static inline void APIENTRY debugCallback(const GLenum source, const GLenum type, const GLuint id,
                                          const GLenum severity, const GLsizei,
                                          const GLchar* const msg, const void* const) {
    // Remove "Buffer detailed info" spam
    if (strstr(msg, "Buffer detailed info")) return;
    // Convert Enums to strings
    char src_str[15];
    switch (source) {
        case gl::DEBUG_SOURCE_WINDOW_SYSTEM:
            strcpy(src_str, "WindowSys");
            break;
        case gl::DEBUG_SOURCE_APPLICATION:
            strcpy(src_str, "App");
            break;
        case gl::DEBUG_SOURCE_API:
            strcpy(src_str, "OpenGL");
            break;
        case gl::DEBUG_SOURCE_SHADER_COMPILER:
            strcpy(src_str, "ShaderCompiler");
            break;
        case gl::DEBUG_SOURCE_THIRD_PARTY:
            strcpy(src_str, "3rdParty");
            break;
        case gl::DEBUG_SOURCE_OTHER:
            strcpy(src_str, "Other");
            break;
        default:
            strcpy(src_str, "Unknown");
    }
    char type_str[12];
    switch (type) {
        case gl::DEBUG_TYPE_ERROR:
            strcpy(type_str, "Error");
            break;
        case gl::DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            strcpy(type_str, "Deprecated");
            break;
        case gl::DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            strcpy(type_str, "Undefined");
            break;
        case gl::DEBUG_TYPE_PORTABILITY:
            strcpy(type_str, "Portability");
            break;
        case gl::DEBUG_TYPE_PERFORMANCE:
            strcpy(type_str, "Performance");
            break;
        case gl::DEBUG_TYPE_MARKER:
            strcpy(type_str, "Marker");
            break;
        case gl::DEBUG_TYPE_PUSH_GROUP:
            strcpy(type_str, "PushGrp");
            break;
        case gl::DEBUG_TYPE_POP_GROUP:
            strcpy(type_str, "PopGrp");
            break;
        case gl::DEBUG_TYPE_OTHER:
            strcpy(type_str, "Other");
            break;
        default:
            strcpy(type_str, "Unknown");
    }
    char severity_str[8];
    switch (severity) {
        case gl::DEBUG_SEVERITY_HIGH:
            strcpy(severity_str, "HIGH");
            break;
        case gl::DEBUG_SEVERITY_MEDIUM:
            strcpy(severity_str, "MEDIUM");
            break;
        case gl::DEBUG_SEVERITY_LOW:
            strcpy(severity_str, "LOW");
            break;
        case gl::DEBUG_SEVERITY_NOTIFICATION:
            strcpy(severity_str, "NOTIFY");
            break;
        default:
            strcpy(severity_str, "UNKNOWN");
            return;
    }
    // Print error information
    printError("%s message: %s[ %s ] ( %d ): %s\n", src_str, type_str, severity_str, id, msg);
}

Window::Window(const int res_x, const int res_y): m_res_x{res_x}, m_res_y{res_y}, m_is_ok{true} {
    glfwSetErrorCallback(errorCallback);
    // Init GLFW
    if (!glfwInit()) {
        m_is_ok = false;
        printError("Error while initializing GLFW.");
        TERMINATE();
    }
    // Select OpenGL 4.4 with a forward compatible core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // Request 32-bit floating-point RGB framebuffer
    glfwWindowHint(GLFW_SRGB_CAPABLE, FALSE);
    glfwWindowHint(GLFW_RED_BITS, 32);
    glfwWindowHint(GLFW_GREEN_BITS, 32);
    glfwWindowHint(GLFW_BLUE_BITS, 32);
    glfwWindowHint(GLFW_ALPHA_BITS, 0);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    #ifdef _DEBUG
        // Enable debug messages
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    #endif
    // Create GLFW window
    m_window = glfwCreateWindow(res_x, res_y, "GLGI", nullptr, nullptr);
    if (!m_window) {
        m_is_ok = false;
        printError("Error while creating GLFW window.");
        glfwTerminate();
        TERMINATE();
    }
    // Set GLFW context
    glfwMakeContextCurrent(m_window);
    // Set key callback
    glfwSetKeyCallback(m_window, keyCallback);
    // Init OpenGL
    const auto success = gl::sys::LoadFunctions();
    if (!success) {
        m_is_ok = false;
        printError("Error while loading OpenGL functions.");
        glfwDestroyWindow(m_window);
        glfwTerminate();
        TERMINATE();
    }
    gl::Enable(gl::DEPTH_TEST);		// Perform depth test
    gl::Enable(gl::CULL_FACE);		// Cull incorrectly-facing triangles (front or back)
    #ifdef _DEBUG
        // Set debug callback
        gl::DebugMessageCallback(debugCallback, nullptr);
        gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE, gl::DONT_CARE, 0, nullptr, GL_TRUE);
        gl::Enable(gl::DEBUG_OUTPUT_SYNCHRONOUS);
    #endif
    // Print OpenGL version information
    const GLubyte* version{gl::GetString(gl::VERSION)};
    if (!version) {
        m_is_ok = false;
        printError("Error while reading OpenGL version.");
        glfwDestroyWindow(m_window);
        glfwTerminate();
        TERMINATE();
    }
    printInfo("OpenGL version: %s.", version);
    // Create accumulation buffer texture
    gl::ActiveTexture(gl::TEXTURE0 + TEX_U_ACCUM);
    // Allocate texture storage
    gl::GenTextures(1, &m_tex_handle);
    gl::BindTexture(gl::TEXTURE_2D, m_tex_handle);
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGB32F, m_res_x, m_res_y, 0, gl::RGB, gl::FLOAT, nullptr);
    // No mipmaps
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAX_LEVEL, 0);
    // No filtering
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
    // Use edge-clamping for both dimensions
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
}

Window::Window(Window&& wnd): m_window{wnd.m_window}, m_res_x{wnd.m_res_x}, m_res_y{wnd.m_res_y},
                              m_tex_handle{wnd.m_tex_handle}, m_is_ok{wnd.m_is_ok} {
    assert(m_is_ok);
    // Mark as moved
    wnd.m_window = nullptr;
}

Window& Window::operator=(Window&& wnd) {
    assert(m_is_ok);
    assert(this != &wnd);
    memcpy(this, &wnd, sizeof(*this));
    // Mark as moved
    wnd.m_window = nullptr;
    return *this;
}

Window::~Window() {
    // Check if it was moved
    if (m_window) {
        gl::DeleteTextures(1, &m_tex_handle);
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}

const bool Window::isOpen() const {
    return m_is_ok;
}

void Window::clear() const {
    gl::Viewport(0, 0, m_res_x, m_res_y);
    gl::BindFramebuffer(gl::FRAMEBUFFER, DEFAULT_FBO);
    gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
    glfwSetTime(0);
}

bool Window::shouldClose() const {
    return (0 != glfwWindowShouldClose(m_window));
}

void Window::refresh() {
    // Copy framebuffer contents to texture
    gl::BindFramebuffer(gl::READ_FRAMEBUFFER, DEFAULT_FBO);
    gl::BindTexture(gl::TEXTURE_2D, m_tex_handle);
    gl::CopyTexImage2D(gl::TEXTURE_2D, 0, gl::RGB32F, 0, 0, m_res_x, m_res_y, 0);
    // Swap buffers
    glfwSwapBuffers(m_window);
    glfwPollEvents();
}

void Window::setTitle(const char* const title) {
    glfwSetWindowTitle(m_window, title);
}

GLFWwindow* Window::get() const {
    return m_window;
}
