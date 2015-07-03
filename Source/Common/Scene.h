#pragma once

#include <vector>
#include <memory>
#include "..\Fog\FogVolume.h"
#include "..\RT\RTBase.h"
#include "..\RT\KdTree.h"
#include "..\GL\GLVertArray.h"
#include "..\GL\GLUniformBuffer.h"

class Scene {
public:
    Scene();
    RULE_OF_ZERO(Scene);
    // Loads objects from *.obj file
    void loadObjects(const char* const file_name, const GLuint prog_handle);
    // Creates fog around the whole scene
    // Loads volume density and preintegrated density data from the specified files,
    // and augments them with the specified coefficients
    void addFog(const char* const dens_file_name, const char* const pi_dens_file_name,
                const float maj_ext_k, const float abs_k, const float sca_k,
                const PerspectiveCamera& cam);
    // Updates absorption and scattering coefficients per unit density,
    // and majorant extinction coefficient of fog
    void updateFogCoeffs(const float maj_ext_k, const float abs_k,
                         const float sca_k);
    // Returns vertex from vector of vertices at specified index
    const glm::vec3& getVertex(const uint index) const;
    // Returns material from vector of materials at specified index
    const rt::PhongMaterial* getMaterial(const uint index) const;
    // Returns extinction coefficient at a given position
    float sampleScaK(const glm::vec3& pos) const;
    // Returns majorant (maximal) extinction coefficient of the fog
    float sampleExtK(const glm::vec3& pos) const;
    // Returns scattering coefficient at a given position
    float getMajExtK() const;
    // Returns scattering albedo - probability of scattering event
    float getScaAlbedo() const;
    // Returns fog bounds
    const BBox& getFogBounds() const;
    // Toggles fog within the scene on and off
    void toggleFog();
    // Traces ray thorough the scene
    bool trace(rt::Ray& ray, const bool is_vis_ray = false) const;
    // Traces ray through fog returning entry and exit distances
    BBox::IntDist traceFog(const rt::Ray& ray) const;
    // Renders scene; if materials are ignored, the whole scene is rendered in one draw call
    void render(const bool ignore_materials = false) const;
private:
    /* OpenGL representation of a scene object */
    struct Object {
        explicit Object(GLVertArray&& va, GLUniformBuffer&& ubo);
        GLVertArray		va;
        GLUniformBuffer ubo;
    };
    GLVertArray			           m_geom_va;       // Contains vertices of the entire scene
    std::vector<Object>            m_objects;       // All objects, combined by material
    std::unique_ptr<FogVolume>     m_fog_vol;       // Heterogeneous fog (if present)
    bool                           m_fog_enabled;   // Flag to toggle fog on/off
    // Raytracing specifics
    std::unique_ptr<rt::KdTri>     m_kd_tree;       // K-d tree spatial accel. structure
    std::vector<glm::vec3>	       m_vertices;      // Vertices for raytracing
    std::vector<glm::vec3>	       m_normals;       // Normals for raytracing
    std::vector<rt::PhongMaterial> m_materials;     // Materials for raytracing
    std::vector<rt::Triangle>      m_triangles;     // Triangles for raytracing
};
