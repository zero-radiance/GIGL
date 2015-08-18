#include "GLRTBLockMngr.h"
#include <cassert>
#include <OpenGL\gl_core_4_4.hpp>

GLRTBLockMngr::GLRingTripleBufferLockManager(): m_buf_idx{0}, m_fences{} {}

GLRTBLockMngr::GLRingTripleBufferLockManager(GLRTBLockMngr&& rtb_lock_mngr):
             m_buf_idx{rtb_lock_mngr.m_buf_idx}, m_fences(std::move(rtb_lock_mngr.m_fences)) {
    // Mark as moved
    rtb_lock_mngr.m_buf_idx = -1;
}

GLRTBLockMngr& GLRTBLockMngr::operator=(GLRTBLockMngr&& rtb_lock_mngr) {
    assert(this != &rtb_lock_mngr);
    // Remove the old fences if needed
    for (int i = 0; i < 3; ++i) {
        gl::DeleteSync(m_fences[i]);
    }
    // Now copy the data
    memcpy(this, &rtb_lock_mngr, sizeof(*this));
    // Mark as moved
    rtb_lock_mngr.m_buf_idx = -1;
    return *this;
}

GLRTBLockMngr::~GLRingTripleBufferLockManager() {
    // Check if it was moved
    if (m_buf_idx >= 0) {
        for (int i = 0; i < 3; ++i) {
            gl::DeleteSync(m_fences[i]);
        }
    }
}

void GLRTBLockMngr::lockBuffer() {
    auto& curr_fence = m_fences[m_buf_idx];
    if (curr_fence) {
        // Release memory
        gl::DeleteSync(curr_fence);
    }
    // Insert a new fence
    curr_fence = gl::FenceSync(gl::SYNC_GPU_COMMANDS_COMPLETE, 0);
    // Prepare the next fence
    m_buf_idx = (m_buf_idx + 1) % 3;
}

void GLRTBLockMngr::waitForLockExpiration() {
    const auto& curr_fence = m_fences[m_buf_idx];
    if (curr_fence) {
        GLbitfield wait_flags   = 0;
        GLuint64   wait_nanosec = 0;
        GLenum     wait_status;
        do {
            wait_status  = gl::ClientWaitSync(curr_fence, wait_flags, wait_nanosec);
            // If wait_status is no good, we have to flush the command buffer
            wait_flags   = gl::SYNC_FLUSH_COMMANDS_BIT;
            wait_nanosec = 1000;
        } while (wait_status != gl::ALREADY_SIGNALED &&
                 wait_status != gl::CONDITION_SATISFIED);
    }
}

int GLRTBLockMngr::getActiveBufIdx() const {
    return m_buf_idx;
}
