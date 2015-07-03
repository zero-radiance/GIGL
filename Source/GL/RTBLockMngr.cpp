#include "RTBLockMngr.h"
#include <cassert>
#include <OpenGL\gl_core_4_4.hpp>

RTBLockMngr::RingTripleBufferLockManager(): m_buf_idx{0}, m_fences{} {}

RTBLockMngr::RingTripleBufferLockManager(RTBLockMngr&& rtb_lock_mngr):
             m_buf_idx{rtb_lock_mngr.m_buf_idx}, m_fences(rtb_lock_mngr.m_fences) {
    // Mark as moved
    rtb_lock_mngr.m_buf_idx = -1;
}

RTBLockMngr& RTBLockMngr::operator=(RTBLockMngr&& rtb_lock_mngr) {
    assert(this != &rtb_lock_mngr);
    memcpy(this, &rtb_lock_mngr, sizeof(*this));
    // Mark as moved
    rtb_lock_mngr.m_buf_idx = -1;
    return *this;
}

RTBLockMngr::~RingTripleBufferLockManager() {
    // Check if it was moved
    if (m_buf_idx >= 0) {
        for (int i = 0; i < 3; ++i) {
            gl::DeleteSync(m_fences[i]);
        }
    }
}

void RTBLockMngr::lockBuffer() {
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

void RTBLockMngr::waitForLockExpiration() {
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
