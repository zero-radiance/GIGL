#pragma once

#include "GLPersistentBuffer.h"

template <GLenum T>
GLPersistentBuffer<T>::GLPersistentBuffer(const size_t byte_sz): m_byte_sz{byte_sz} {
    gl::GenBuffers(1, &m_handle);
    // Allocate buffer on GPU
    gl::BindBuffer(T, m_handle);
    static const GLbitfield map_flags{gl::MAP_PERSISTENT_BIT |  // Persistent flag
                                      gl::MAP_COHERENT_BIT   |  // Make changes auto. visible to GPU
                                      gl::MAP_WRITE_BIT };      // Map it for writing
    gl::BufferStorage(T, m_byte_sz, nullptr, map_flags);
    // Map the data
    m_buffer = gl::MapBufferRange(T, 0, m_byte_sz, map_flags);
}

template <GLenum T>
GLPersistentBuffer<T>::GLPersistentBuffer(const GLPersistentBuffer& pbo): m_byte_sz{pbo.m_byte_sz} {
    gl::GenBuffers(1, &m_handle);
    // Allocate buffer on GPU
    gl::BindBuffer(T, m_handle);
    static const GLbitfield map_flags{gl::MAP_PERSISTENT_BIT |  // Persistent flag
                                      gl::MAP_COHERENT_BIT   |  // Make changes auto. visible to GPU
                                      gl::MAP_WRITE_BIT };      // Map it for writing
    gl::BufferStorage(T, m_byte_sz, nullptr, map_flags);
    // Map the data
    m_buffer = gl::MapBufferRange(T, 0, m_byte_sz, map_flags);
}

template <GLenum T>
GLPersistentBuffer<T>& GLPersistentBuffer<T>::operator=(const GLPersistentBuffer& pbo) {
    m_byte_sz = pbo.m_byte_sz;
    gl::GenBuffers(1, &m_handle);
    // Allocate buffer on GPU
    gl::BindBuffer(T, m_handle);
    static const GLbitfield map_flags{gl::MAP_PERSISTENT_BIT |  // Persistent flag
                                      gl::MAP_COHERENT_BIT |  // Make changes auto. visible to GPU
                                      gl::MAP_WRITE_BIT};      // Map it for writing
    gl::BufferStorage(T, m_byte_sz, nullptr, map_flags);
    // Map the data
    m_buffer = gl::MapBufferRange(T, 0, m_byte_sz, map_flags);
    return *this;
}

template <GLenum T>
GLPersistentBuffer<T>::GLPersistentBuffer(GLPersistentBuffer&& pbo): m_buffer{pbo.m_buffer},
                                                                     m_byte_sz{pbo.m_byte_sz},
                                                                     m_handle{pbo.m_handle} {
    // Mark as moved
    pbo.m_handle = 0;
}


template <GLenum T>
GLPersistentBuffer<T>& GLPersistentBuffer<T>::operator=(GLPersistentBuffer&& pbo) {
    memcpy(this, &pbo, sizeof(*this));
    // Mark as moved
    pbo.m_handle = 0;
    return *this;
}

template <GLenum T>
GLPersistentBuffer<T>::~GLPersistentBuffer() {
    // Check if it was moved
    if (m_handle) {
        gl::BindBuffer(T, m_handle);
        gl::UnmapBuffer(T);
        gl::DeleteBuffers(1, &m_handle);
    }
}

template <GLenum T>
void* GLPersistentBuffer<T>::data() {
    return m_buffer;
}

template <GLenum T>
const void* GLPersistentBuffer<T>::data() const {
    return m_buffer;
}

template <GLenum T>
void GLPersistentBuffer<T>::bind(const GLuint bind_idx) const {
    gl::BindBufferBase(T, bind_idx, m_handle);
}
