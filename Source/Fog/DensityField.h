#pragma once

#include <GLM\vec3.hpp>
#include <OpenGL\gl_basic_typedefs.h>
#include "..\Common\BBox.h"

class Scene;
class PerspectiveCamera;

/* Scalar-valued particle density field */
class DensityField {
public:
    DensityField() = delete;
    RULE_OF_FIVE(DensityField);
    // Constructor that generates density field using simplex noise
    explicit DensityField(const BBox& bb, const int(&res)[3], const float freq, const float ampl,
                          const PerspectiveCamera& cam, const Scene& scene);
    // Constructor that reads density field and preintegrated density values from .3dt files
    explicit DensityField(const BBox& bb, const char* const dens_file_name,
                          const char* const pi_dens_file_name);
    // Returns bounding box/volume
    const BBox& bbox() const;
    // Returns bounding volume entry and exit distances
    BBox::IntDist intersect(const rt::Ray& ray) const;
    // Samples density at a given spatial position
    float sampleDensity(const glm::vec3& pos) const;
private:
    // Returns a density sample
    float sample(const GLsizei x, const GLsizei y, const GLsizei z) const;
    // Reads density values from file
    void read(const char* const file_name);
    // Writes density values to file
    void write(const char* const file_name) const;
    // Returns the size of the field, e.i. the number of stored density values
    GLsizei size() const;
    // Creates a density texture in OpenGL
    void createTex();
    // Computes a 3D-texture with preintegrated camera-space density values
    // Numerically valuate e ^ (Int{0..d}(density(t))dt)
    void computePiDensity(const PerspectiveCamera& cam, const Scene& scene);
    // Reads preintegrated density values from file
    void readPiDens(const char* const file_name);
    // Writes preintegrated density values to file
    void writePiDens(const char* const file_name) const;
    // Returns the size of preintegrated density data
    GLsizei piDensSize() const;
    // Creates a preintegrated density texture in OpenGL
    void createPiDensTex();
    // Performs object destruction
    void destroy();
    // Private data members
    BBox       m_bbox;                  // Bounding box/volume
    glm::ivec3 m_res;                   // Resolution in X-Y-Z
    GLubyte*   m_data;                  // Scalar density data
    GLuint     m_tex_handle;            // Density texture OpenGL handle
    glm::ivec3 m_pi_dens_res;           // Resolution of preintegrated density in X-Y-Z
    GLfloat*   m_pi_dens_data;          // Preintegrated density data
    GLuint     m_pi_dens_tex_handle;    // Preintegrated density texture OpenGL handle
};
