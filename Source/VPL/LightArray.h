#pragma once

#include "..\GL\GLPersistentBuffer.h"

/* Array of Point Lights */
template <class PL>
class LightArray {
public:
    LightArray() = delete;
    RULE_OF_ZERO(LightArray);
    // Constructs a light array capable of storing max_n_lights (active) lights
    LightArray(const int max_n_lights);
    // Returns size of PL array
    int size() const;
    // Returns a mutable PL reference from array
    PL& operator[](const int index);
    // Returns a const PL reference from array
    const PL& operator[](const int index) const;
    // Resets the size of the array, effectively clearing it
    void clear();
    // Returns true if array contains 0 lights
    bool isEmpty() const;
    // Adds a single PL to array
    void addLight(const PL& pl);
    // Normalizes intensity by (1 / n_paths)
    void normalizeIntensity(const int n_paths);
    // Binds uniform buffer object to to uniform buffer binding point
    void bind(const GLuint bind_idx) const;
    // Activates the next buffer in ring-triple-buffer
    void switchToNextBuffer();
private:
    // Returns the pointer to the light buffer
    PL* data();
    // Returns the pointer to the light buffer (read-only)
    const PL* data() const;
    // Private data members
    GLPUB140 m_ubo;         // OpenGL persistent uniform buffer object
    int      m_capacity;    // Total buffer capacity (max. possible number of active lights)
    int      m_sz;          // Numer of stored (active) lights
    int      m_offset;      // Offset to the beginning of the current buffer
};
