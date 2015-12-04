#include "Scene.h"
#include <TinyOBJ\tiny_obj_loader.h>
#include <GLM\gtx\normal.hpp>
#include "Constants.h"
#include "..\RT\KdTree.hpp"
#include "..\GL\GLPersistentBuffer.hpp"

using glm::vec3;
using glm::uvec3;
using glm::normalize;

CONSTEXPR GLsizei n_mesh_attr         = 2;              // Position, normal
CONSTEXPR GLsizei mesh_attr_lengths[] = {3, 3};         // vec3, vec3

Scene::Scene(): m_geom_va{1, mesh_attr_lengths},
                m_material_pbo{MAX_MATERIALS * sizeof(rt::PhongMaterial)},
                m_fog_vol{nullptr}, m_fog_enabled{false}, m_kd_tree{nullptr} {
    m_material_pbo.bind(UB_MAT_ARR);
}

void Scene::loadObjects(const char* const file_name) {
    // Copy path from filename
    const char* const last_backslash_pos{strrchr(file_name, '\\')};
    const auto path_len = last_backslash_pos - file_name + 1;    // + 1 for '\\'
    char* const path{new char[path_len + 1]};					 // + 1 for '\0'
    strncpy(path, file_name, path_len);
    path[path_len] = '\0';										// Null-terminating
    std::vector<tinyobj::shape_t> shapes;
    tinyobj::LoadObj(shapes, file_name, path);
    if (shapes.empty()) {
        printError("Object file loading failed!");
        TERMINATE();
    }
    // Sort shapes by material
    auto cmp_shapes_by_material = [](const tinyobj::shape_t& lhs,
                                     const tinyobj::shape_t& rhs) {
        return lhs.material.name < rhs.material.name;
    };
    std::sort(shapes.begin(), shapes.end(), cmp_shapes_by_material);
    // Process loaded shapes and transform them to objects
    // Combine them by materials
    uint global_vert_offset{0};
    uint object_vert_offset{0};
    Object* curr_object{nullptr};
    uint    curr_mat_id{0};
    const std::string* curr_mat_name{nullptr};
    for (auto s = shapes.begin(); s != shapes.end(); ++s) {
        const uint vert_count{static_cast<uint>(s->mesh.positions.size()) / 3};
        if (s->mesh.normals.empty()) {
            // Generate normals
            for (uint i = 0; i < vert_count; ++i) {
                uint indices[3];
                for (uint k = 0, e = static_cast<uint>(s->mesh.indices.size()); k < e; ++k) {
                    if (i == s->mesh.indices[k]) {
                        const uint rem{k % 3};
                        indices[rem] = s->mesh.indices[k];
                        switch (rem) {
                            case 0:
                                indices[1] = s->mesh.indices[k + 1];
                                indices[2] = s->mesh.indices[k + 2];
                                break;
                            case 1:
                                indices[0] = s->mesh.indices[k - 1];
                                indices[2] = s->mesh.indices[k + 1];
                                break;
                            case 2:
                                indices[0] = s->mesh.indices[k - 2];
                                indices[1] = s->mesh.indices[k - 1];
                                break;
                        }
                        break;
                    }
                }
                vec3 vertices[3];
                for (auto k = 0; k < 3; ++k) {
                    vertices[k] = vec3{s->mesh.positions[3 * indices[k]],
                        s->mesh.positions[3 * indices[k] + 1],
                        s->mesh.positions[3 * indices[k] + 2]};
                }
                const vec3 normal{glm::triangleNormal(vertices[0], vertices[1], vertices[2])};
                for (auto k = 0; k < 3; ++k) {
                    s->mesh.normals.emplace_back(normal[k]);
                }
            }
        }
        if (!curr_mat_name || *curr_mat_name != s->material.name) {
            // New material => new object
            if (curr_mat_name) {
                ++curr_mat_id;
                // Buffer the previous object
                m_objects.back().va.buffer();
                m_objects.back().ebo.buffer();
            }
            curr_mat_name = &s->material.name;
            // Create a new object
            GLVertArray		va{n_mesh_attr, mesh_attr_lengths};
            GLElementBuffer ebo{};
            m_objects.emplace_back(curr_mat_id, std::move(va), std::move(ebo));
            curr_object = &m_objects[m_objects.size() - 1];
            object_vert_offset = 0;
            // Set material properties
            vec3 rho_d{s->material.diffuse[0], s->material.diffuse[1], s->material.diffuse[2]};
            vec3 rho_s{s->material.specular[0], s->material.specular[1], s->material.specular[2]};
            // Normalize s.t. rho_d + rho_s <= vec3(1.0f)
            const vec3 sum{rho_d + rho_s};
            if (sum.r > 1.0f || sum.g > 1.0f || sum.b > 1.0f) {
                rho_d /= sum;
                rho_s /= sum;
            }
            const vec3    k_d{rho_d * INV_PI};
            const GLfloat n_s{s->material.shininess};
            const vec3    k_s{rho_s * (n_s + 2.0f) * 0.5f * INV_PI};
            const vec3    k_e{s->material.emission[0], s->material.emission[1],
                              s->material.emission[2]};
            // Store material coefficients
            auto* materials = static_cast<rt::PhongMaterial*>(m_material_pbo.data());
            materials[curr_mat_id] = rt::PhongMaterial{k_d, k_s, n_s, k_e};
        }
        auto& object_va  = curr_object->va;
        auto& object_ebo = curr_object->ebo;
        // Load mesh
        object_va.loadData(0, s->mesh.positions);
        m_geom_va.loadData(0, s->mesh.positions);
        object_va.loadData(1, s->mesh.normals);
        // Load indexing information
        object_ebo.loadData(s->mesh.indices, object_vert_offset);
        m_geom_ebo.loadData(s->mesh.indices, global_vert_offset);
        if (shapes.end() == s + 1) {
            // Don't forget to buffer after the last shape
            object_va.buffer();
            m_geom_va.buffer();
            object_ebo.buffer();
            m_geom_ebo.buffer();
        }
        // Fill vectors for raytracing
        const uint start_idx{static_cast<uint>(m_vertices.size())};
        for (size_t k = 0, e = s->mesh.positions.size(); k < e; k += 3) {
            m_vertices.emplace_back(s->mesh.positions[k],
                                    s->mesh.positions[k + 1],
                                    s->mesh.positions[k + 2]);
        }
        for (size_t k = 0, e = s->mesh.normals.size(); k < e; k += 3) {
            m_normals.emplace_back(s->mesh.normals[k],
                                   s->mesh.normals[k + 1],
                                   s->mesh.normals[k + 2]);
        }
        // Create triangles
        for (size_t k = 0, e = s->mesh.indices.size(); k < e; k += 3) {
            const uvec3 vert = {start_idx + s->mesh.indices[k],
                                start_idx + s->mesh.indices[k + 1],
                                start_idx + s->mesh.indices[k + 2]};
            m_triangles.emplace_back(vert, curr_mat_id);
        }
        object_vert_offset += vert_count;
        global_vert_offset += vert_count;
    }
    // Build acceleration structure
    m_kd_tree = std::make_unique<rt::KdTri>(m_triangles, 10, 1, 30, 2);
}

Scene::Object::Object(const uint material_id, GLVertArray&& va, GLElementBuffer&& ebo):
               mat_id{material_id}, va{va}, ebo{ebo} {}

void Scene::addFog(const char* const dens_file_name, const char* const pi_dens_file_name,
                   const float maj_ext_k, const float abs_k, const float sca_k,
                   const PerspectiveCamera& cam) {
    vec3 pt_max{m_kd_tree->bbox().maxPt()};
    // Limit fog height to the top of the box
    pt_max.y = std::min(pt_max.y, MAX_FOG_HEIGHT);
    const BBox bb{m_kd_tree->bbox().minPt(), pt_max};
    // Check whether the assets exist
    bool file_exists;
    if (auto file = fopen(dens_file_name, "rb")) {
        fclose(file);
        file_exists = true;
    } else {
        file_exists = false;
    }
    if (file_exists) {
        // Load fog info from disk
        m_fog_vol = std::make_unique<FogVolume>(DensityField{bb, dens_file_name, pi_dens_file_name},
                                                maj_ext_k, abs_k, sca_k);
    } else {
        // Generate new fog from scratch
        static const int res[3] = {64, 64, 64};
        static const float freq = 12.0f;
        static const float ampl = 1.0f;
        m_fog_vol = std::make_unique<FogVolume>(DensityField{bb, res, freq, ampl, cam, *this},
                                                maj_ext_k, abs_k, sca_k);
    }
    m_fog_enabled = true;
}

void Scene::updateFogCoeffs(const float maj_ext_k, const float abs_k,
                            const float sca_k) {
    assert(m_fog_vol);
    m_fog_vol->setCoeffs(maj_ext_k, abs_k, sca_k);
}

const vec3& Scene::getVertex(const uint index) const {
    return m_vertices[index];
}

const rt::PhongMaterial* Scene::getMaterial(const uint index) const {
    const auto* materials = static_cast<const rt::PhongMaterial*>(m_material_pbo.data());
    return &materials[index];
}

float Scene::sampleScaK(const vec3& pos) const {
    return m_fog_vol->sampleScaK(pos);
}

float Scene::sampleExtK(const vec3& pos) const {
    return m_fog_vol->sampleExtK(pos);
}

float Scene::getMajExtK() const {
    return m_fog_vol->getMajExtK();
}

float Scene::getScaAlbedo() const {
    return m_fog_vol->getScaAlbedo();
}

const BBox& Scene::getFogBounds() const {
    return m_fog_vol->bbox();
}

void Scene::toggleFog() {
    assert(m_fog_vol);
    m_fog_enabled = !m_fog_enabled;
}

bool Scene::trace(rt::Ray& ray, const bool is_vis_ray) const {
    // Traverse the tree
    if (m_kd_tree->intersect(ray, is_vis_ray)) {
        if (is_vis_ray) { return true; }
        ray.inters.normal = normalize(ray.inters.normal);
        return true;
    } else {
        return false;
    }
}

BBox::IntDist Scene::traceFog(const rt::Ray& ray) const {
    if (m_fog_enabled) {
        return m_fog_vol->intersect(ray);
    } else {
        return BBox::IntDist{};
    }
}

void Scene::render(const bool ignore_materials) const {
    if (ignore_materials) {
        m_geom_ebo.draw(m_geom_va);
    } else {
        for (const auto& obj : m_objects) {
            gl::Uniform1i(UL_GB_MAT_ID, obj.mat_id);
            obj.ebo.draw(obj.va);
        }
    }
}
