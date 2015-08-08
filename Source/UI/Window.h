#pragma once

#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

struct GLFWwindow;

/* Class wrapping around GLFW window */
class Window {
public:
    Window() = delete;
    RULE_OF_FIVE_NO_COPY(Window);
    // Constructs a window with specified resolution
    Window(const int res_x, const int res_y);
    // Returns true if window has been opened successfully
    const bool isOpen() const;
    // Clears window and associated buffers
    void clear() const;
    // Returns true if closing sequence has been triggered
    bool shouldClose() const;
    // Refreshes image, swaps buffers
    void refresh();
    // Sets window title to a specified string
    void setTitle(const char* const title);
    // Returns a pointer to GLFWwindow
    GLFWwindow* get() const;
private:
    // Performs window destruction
    void destroy();
    // Private data members
    GLFWwindow* m_window;           // Pointer to GLFW window
    GLsizei		m_res_x, m_res_y;   // Resolution in x, y
    GLuint		m_tex_handle;       // OpenGL accumulation texture handle
    bool		m_is_ok;            // Status
};
