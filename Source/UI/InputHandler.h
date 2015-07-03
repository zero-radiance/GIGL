#pragma once

#include <GLM\vec3.hpp>
#include "..\Common\Definitions.h"

struct GLFWwindow;

/* Rendering parameters controlled by InputHandler */
struct RenderParams {
    RenderParams() = default;
    RULE_OF_ZERO(RenderParams);
    bool      gi_enabled;    // Indicates whether Global Illumination is enabled
    bool      clamp_r_sq;    // Indicates whether radius squared of VPLs is being clamped
    int       exposure;      // Exposure time; higher values increase brightness
    int       frame_num;     // Current frame number
    uint      curr_time_ms;  // Number of milliseconds since timer reset (curr. frame)
    int       max_num_vpls;  // Maximal number of active VPLs
    float     abs_k;         // Absorption coefficient per unit density
    float     sca_k;         // Scattering coefficient per unit density
    float     maj_ext_k;     // Majorant extinction coefficient of fog
    glm::vec3 ppl_w_pos;     // Primary point light's position in world coordinates
};

/* Static class handling user HID input */
class InputHandler {
public:
    InputHandler() = delete;
    RULE_OF_ZERO(InputHandler);
    // Aquires and initializes (resets) all controlled parameters
    static void init(RenderParams* params);
    // Adjusts parameters based on HID input
    static void updateParams(GLFWwindow* const wnd);
private:
    // Handle toggling options on/off
    static void handleToggles(GLFWwindow* const wnd);
    // Handle input like motion controls and parameter adjustment
    static void handleSmoothInput(GLFWwindow* const wnd);
    // Resets all status to defaults
    static void reset();
    // Updates last toggle time
    static void updateLastTime();
    // Resets current frame number to 0
    static void resetFrameCount();
    // Private data members
    static RenderParams* m_params;  // Controlled parameters
    static uint m_last_time_ms;     // Number of milliseconds since timer reset (last toggle)
};
