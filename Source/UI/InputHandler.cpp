#include "InputHandler.h"
#include "..\Common\Timer.h"
#include "..\Common\Constants.h"
#include "..\Common\Renderer.h"
#include "..\Common\Scene.h"
#include <GLFW\glfw3.h>

using glm::vec3;

extern Scene* scene;

RenderSettings* InputHandler::m_params;
uint InputHandler::m_last_time_ms;

void InputHandler::init(RenderSettings* params) {
    m_params = params;
    reset();
}

void InputHandler::updateParams(GLFWwindow* const wnd) {
    m_params->curr_time_ms = HighResTimer::time_ms();
    handleToggles(wnd);
    handleSmoothInput(wnd);
}

void InputHandler::handleToggles(GLFWwindow* const wnd) {
    const bool ignore_input{m_params->curr_time_ms - m_last_time_ms < THRESHOLD_MS};
    if (!ignore_input) {
        if (glfwGetKey(wnd, GLFW_KEY_SPACE)) {
            // Reset everything
            reset();
        } else if (glfwGetKey(wnd, GLFW_KEY_G)) {
            // Toggle global illumination
            m_params->gi_enabled = !m_params->gi_enabled;
            updateLastTime();
            resetFrameCount();
        } else if (glfwGetKey(wnd, GLFW_KEY_C)) {
            // Toggle VPL radius clamping
            m_params->clamp_r_sq = !m_params->clamp_r_sq;
            updateLastTime();
            resetFrameCount();
        } else if (glfwGetKey(wnd, GLFW_KEY_F)) {
            // Toggle fog
            if (m_params->maj_ext_k != 0.0f) {
                m_params->maj_ext_k = m_params->sca_k = m_params->abs_k = 0.0f;
            } else {
                m_params->abs_k     = ABS_K;
                m_params->sca_k     = SCA_K;
                m_params->maj_ext_k = MAJ_EXT_K;
            }
            scene->toggleFog();
            scene->updateFogCoeffs(m_params->maj_ext_k, m_params->abs_k, m_params->sca_k);
            updateLastTime();
            resetFrameCount();
        }
    }
}

void InputHandler::handleSmoothInput(GLFWwindow* const wnd) {
    const float delta_t{static_cast<float>(glfwGetTime())};
    const float delta_pos{100.0f * delta_t};
    if (glfwGetKey(wnd, GLFW_KEY_LEFT)) {
        // Move light to the left
        m_params->ppl_w_pos.x += delta_pos;
        resetFrameCount();
    } else if (glfwGetKey(wnd, GLFW_KEY_RIGHT)) {
        // Move light to the right
        m_params->ppl_w_pos.x -= delta_pos;
        resetFrameCount();
    }
    if (glfwGetKey(wnd, GLFW_KEY_UP)) {
        // Move light up
        m_params->ppl_w_pos.y += delta_pos;
        resetFrameCount();
    } else if (glfwGetKey(wnd, GLFW_KEY_DOWN)) {
        // Move light down
        m_params->ppl_w_pos.y -= delta_pos;
        resetFrameCount();
    }
    if (glfwGetKey(wnd, GLFW_KEY_KP_ADD)) {
        // Make fog denser
        m_params->abs_k     *= 1.1f;
        m_params->sca_k     *= 1.1f;
        m_params->maj_ext_k *= 1.1f;
        scene->updateFogCoeffs(m_params->maj_ext_k, m_params->abs_k, m_params->sca_k);
        resetFrameCount();
    } else if (glfwGetKey(wnd, GLFW_KEY_KP_SUBTRACT)) {
        // Make fog thinner
        m_params->abs_k     *= 0.9f;
        m_params->sca_k     *= 0.9f;
        m_params->maj_ext_k *= 0.9f;
        scene->updateFogCoeffs(m_params->maj_ext_k, m_params->abs_k, m_params->sca_k);
        resetFrameCount();
    }
    if (glfwGetKey(wnd, GLFW_KEY_KP_MULTIPLY)) {
        // Increase exposure time
        m_params->exposure += 1;
        resetFrameCount();
    } else if (glfwGetKey(wnd, GLFW_KEY_KP_DIVIDE)) {
        // Reduce exposure time
        m_params->exposure -= 1;
        resetFrameCount();
    }
    if (glfwGetKey(wnd, GLFW_KEY_1)) {
        // Set the maximal number of active VPLs to 10
        m_params->max_num_vpls = 10;
        resetFrameCount();
    } else if (glfwGetKey(wnd, GLFW_KEY_2)) {
        // Set the maximal number of active VPLs to 50
        m_params->max_num_vpls = 50;
        resetFrameCount();
    } else if (glfwGetKey(wnd, GLFW_KEY_3)) {
        // Set the maximal number of active VPLs to MAX_N_VPLS
        m_params->max_num_vpls = MAX_N_VPLS;
        resetFrameCount();
    } else if (glfwGetKey(wnd, GLFW_KEY_R)) {
        // Reset frame count
        resetFrameCount();
    }
}

void InputHandler::reset() {
    // Reset m_params
    m_params->gi_enabled   = false;
    m_params->clamp_r_sq   = true;
    m_params->exposure     = 16;
    m_params->frame_num    = 0;
    m_params->max_num_vpls = MAX_N_VPLS;
    m_params->abs_k        = ABS_K;
    m_params->sca_k        = SCA_K;
    m_params->maj_ext_k    = MAJ_EXT_K;
    m_params->ppl_w_pos    = PRIM_PL_POS;
    // Reset timer
    m_last_time_ms = 0;
}

void InputHandler::updateLastTime() {
    m_last_time_ms = m_params->curr_time_ms;
}

void InputHandler::resetFrameCount() {
    m_params->frame_num = 0;
}
