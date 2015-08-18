#pragma once

#include "..\Common\Definitions.h"

struct GLFWwindow;
struct RenderSettings;

/* Static class handling user HID input */
class InputHandler {
public:
    InputHandler() = delete;
    RULE_OF_ZERO(InputHandler);
    // Aquires and initializes (resets) all controlled parameters
    static void init(RenderSettings* params);
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
    static RenderSettings* m_params;    // Controlled parameters
    static uint m_last_time_ms;         // Number of milliseconds since timer reset (last toggle)
};
