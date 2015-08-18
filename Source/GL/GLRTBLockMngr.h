#pragma once

#include <array>
#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\Definitions.h"

/* Implements synchronization for ring-triple-buffers */
class GLRingTripleBufferLockManager {
public:
    GLRingTripleBufferLockManager();
    RULE_OF_FIVE_NO_COPY(GLRingTripleBufferLockManager);
    // Prevents writing to buffer (1/3) until GPU commands pipeleine finishes execution
    void lockBuffer();
    // Waits for fence removal (write access)
    void waitForLockExpiration();
    // Returns the active ring buffer index: [0..2]
    int getActiveBufIdx() const;
private:
    std::array<GLsync, 3> m_fences;     // Synchronization for ring-triple-buffer
    int                   m_buf_idx;    // Active ring buffer index: [0..2]
};

using GLRTBLockMngr = GLRingTripleBufferLockManager;
