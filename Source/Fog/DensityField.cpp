#include "DensityField.h"
#include <GLM\gtc\noise.hpp>
#include <OpenGL\gl_core_4_4.hpp>
#include "..\Common\Constants.h"
#include "..\Common\Utility.hpp"
#include "..\Common\Interpolation.hpp"
#include "..\Common\Camera.h"
#include "..\Common\Scene.h"

using glm::ivec2;
using glm::vec3;
using glm::ivec3;
using glm::min;
using glm::max;
using glm::ceil;
using glm::floor;

DensityField::DensityField(const BBox& bb, const int(&res)[3], const float freq, const float ampl,
                           const PerspectiveCamera& cam, const Scene& scene):
                           m_bbox{bb}, m_res{res[0], res[1], res[2]},
                           m_data{new GLubyte[res[0] * res[1] * res[2]]},
                           m_pi_dens_data{nullptr} {
    // Validate parameters
    assert(m_res.x > 0 && m_res.y > 0 && m_res.z > 0);
    assert(freq > 0.0f && 0.0f < ampl && ampl <= 1.0f);
    // Initialize maximal density
    float max_dens{0.0f};
    // Compute normalization factors
    const float x_norm{1.0f / (m_res.x - 1)};
    const float y_norm{1.0f / (m_res.y - 1)};
    const float z_norm{1.0f / (m_res.z - 1)};
    printInfo("The renderer has been started for the first time.");
    printInfo("Procedurally computing fog density values using simplex noise.");
    // Compute per-pixel noise values
    #pragma omp parallel for
    for (int z = 0; z < m_res[2]; ++z)
    for (int y = 0; y < m_res[1]; ++y)
    for (int x = 0; x < m_res[0]; ++x) {
        const vec3 norm_pos{x * x_norm, y * y_norm, z * z_norm};
        float sum{0.0f};
        float curr_freq{freq};
        float curr_ampl{ampl};
        // Compute value for each octave
        for (int oct = 0; oct < N_OCTAVES; ++oct) {
            // Value in range [-1, 1]
            float val{glm::simplex(curr_freq * norm_pos)};
            // Now mapped to [0, 0.5]
            val = 0.25f * (val + 1.0f);
            // curr_ampl <= 1
            // Therefore, sum is range [0, 1): at most 0.5 + 0.25 + 0.125 + ...
            sum += curr_ampl * val;
            // Double the frequency, half the amplitude
            curr_freq *= 2.0f;
            curr_ampl /= 2.0f;
        }
        const float   dens_val{sum / N_OCTAVES};
        const GLubyte byte_dens{static_cast<GLubyte>(255.0f * dens_val)};
        max_dens = max(max_dens, dens_val);
        m_data[x + y * m_res.x + z * m_res.x * m_res.y] = byte_dens;
    }
    const float inv_max_dens{1.0f / max_dens};
    // Initialize minimal, average, maximal densities
    float min_dens{FLT_MAX}, avg_dens{0.0f};
    max_dens = 0.0f;
    // Linearly rescale the values
    for (int z = 0; z < m_res[2]; ++z)
    for (int y = 0; y < m_res[1]; ++y)
    for (int x = 0; x < m_res[0]; ++x) {
        const GLubyte old_byte_dens{m_data[x + y * m_res.x + z * m_res.x * m_res.y]};
        const float   new_dens{min(old_byte_dens * inv_max_dens / 255.0f, 1.0f)};
        const GLubyte new_byte_dens{static_cast<GLubyte>(255.0f * new_dens)};
        min_dens  = min(min_dens, new_dens);
        max_dens  = max(max_dens, new_dens);
        avg_dens += new_dens;
        m_data[x + y * m_res.x + z * m_res.x * m_res.y] = new_byte_dens;
    }
    #ifndef NDEBUG
        printInfo("Minimal density: %.2f", min_dens);
        printInfo("Maximal density: %.2f", max_dens);
        printInfo("Average density: %.2f", avg_dens / static_cast<float>(m_res.x * m_res.y * m_res.z));
    #endif
    // Save it to disk
    write("Assets\\df.3dt");
    // Load data into OpenGL texture
    createTex();
    // Preintegrate density values along primary rays
    computePiDensity(cam, scene);
}

DensityField::DensityField(const BBox& bb, const char* const dens_file_name,
                           const char* const pi_dens_file_name): m_bbox{bb} {
    read(dens_file_name);
    createTex();
    readPiDens(pi_dens_file_name);
    createPiDensTex();
}

DensityField::DensityField(const DensityField& df): m_bbox{df.m_bbox}, m_res{df.m_res},
                                                    m_data{new GLubyte[df.size()]},
                                                    m_pi_dens_res{df.m_pi_dens_res},
                                                    m_pi_dens_data{df.m_pi_dens_data} {
    memcpy(m_data, df.m_data, df.size() * sizeof(GLubyte));
    createTex();
    if (m_pi_dens_data) {
        m_pi_dens_data = new GLfloat[df.piDensSize()];
        memcpy(m_pi_dens_data, df.m_pi_dens_data, df.piDensSize() * sizeof(GLfloat));
        createPiDensTex();
    }
}

DensityField& DensityField::operator=(const DensityField& df) {
    if (this != &df) {
        // Release memory
        destroy();
        // Now copy the data
        m_bbox = df.m_bbox;
        m_res  = df.m_res;
        m_data = new GLubyte[df.size()];
        m_pi_dens_res = df.m_pi_dens_res;
        memcpy(m_data, df.m_data, df.size() * sizeof(GLubyte));
        createTex();
        if (df.m_pi_dens_data) {
            m_pi_dens_data = new GLfloat[df.piDensSize()];
            memcpy(m_pi_dens_data, df.m_pi_dens_data, df.piDensSize() * sizeof(GLfloat));
            createPiDensTex();
        } else {
            m_pi_dens_data = nullptr;
        }
    }
    return *this;
}

DensityField::DensityField(DensityField&& df): m_bbox{df.m_bbox}, m_res{df.m_res},
                                               m_data{df.m_data}, m_tex_handle{df.m_tex_handle},
                                               m_pi_dens_res{df.m_pi_dens_res},
                                               m_pi_dens_data{df.m_pi_dens_data},
                                               m_pi_dens_tex_handle{df.m_pi_dens_tex_handle} {
    // Mark as moved
    df.m_tex_handle = 0;
}

DensityField& DensityField::operator=(DensityField&& df) {
    assert(this != &df);
    // Release memory
    destroy();
    // Now copy the data
    memcpy(this, &df, sizeof(*this));
    // Mark as moved
    df.m_tex_handle = 0;
    return *this;
}

DensityField::~DensityField() {
    // Check if it was moved
    if (m_tex_handle) {
        destroy();
    }
}

void DensityField::destroy() {
    gl::DeleteTextures(1, &m_tex_handle);
    delete[] m_data;
    if (m_pi_dens_data) {
        gl::DeleteTextures(1, &m_pi_dens_tex_handle);
        delete[] m_pi_dens_data;
    };
}

const BBox& DensityField::bbox() const {
    return m_bbox;
}

BBox::IntDist DensityField::intersect(const rt::Ray& ray) const {
    return m_bbox.intersect(ray);
}

float DensityField::sampleDensity(const vec3& pos) const {
    // Compute normalized position [0..1]^3
    const vec3 n_pos{m_bbox.computeNormPos(pos)};
    // Compute texel coordinate
    vec3 tex_coord{n_pos * vec3{m_res}};
    // Use voxel centers as texel values, just as OpenGL does
    tex_coord -= vec3{0.5f};
    // Compute pixel coordinates
    const int x[2] = {static_cast<int>(floor(tex_coord.x)), static_cast<int>(ceil(tex_coord.x))};
    const int y[2] = {static_cast<int>(floor(tex_coord.y)), static_cast<int>(ceil(tex_coord.y))};
    const int z[2] = {static_cast<int>(floor(tex_coord.z)), static_cast<int>(ceil(tex_coord.z))};
    // Obtain neighbouring samples
    const float tx{tex_coord.x - x[0]};
    const float ty{tex_coord.y - y[0]};
    const float tz{tex_coord.z - z[0]};
    const float d000{sample(x[0], y[0], z[0])};
    const float d100{sample(x[1], y[0], z[0])};
    const float d010{sample(x[0], y[1], z[0])};
    const float d110{sample(x[1], y[1], z[0])};
    const float d001{sample(x[0], y[0], z[1])};
    const float d101{sample(x[1], y[0], z[1])};
    const float d011{sample(x[0], y[1], z[1])};
    const float d111{sample(x[1], y[1], z[1])};
    // Perform trilinear interpolation
    return lerp3D(d000, d100, d010, d110, d001, d101, d011, d111, tx, ty, tz);
}

float DensityField::sample(const GLsizei x, const GLsizei y, const GLsizei z) const {
    if (x >= 0 && x < m_res.x &&
        y >= 0 && y < m_res.y &&
        z >= 0 && z < m_res.z ) {
        return m_data[x + y * m_res.x + z * m_res.x * m_res.y] / 255.0f;
    }
    return 0.0f;
}

void DensityField::read(const char* const file_name) {
    // Open file
    auto file = fopen(file_name, "rb");
    if (!file) {
        // Something went wrong
        printError("Failed to open volume density file %s for reading.", file_name);
        TERMINATE();
    }
    // Read resolution
    fread(&m_res, sizeof(GLsizei), 3, file);
    // Read image data
    const GLsizei n_values{size()};
    m_data = new GLubyte[n_values];
    fread(m_data, sizeof(GLubyte), n_values, file);
    // Close file
    fclose(file);
}

void DensityField::write(const char* const file_name) const {
    // Open file
    auto file = fopen(file_name, "wb");
    if (!file) {
        // Something went wrong
        printError("Failed to open volume density file %s for writing.", file_name);
        TERMINATE();
    }
    // Write resolution
    fwrite(&m_res, sizeof(GLsizei), 3, file);
    // Write image data
    const GLsizei n_values{size()};
    fwrite(m_data, sizeof(GLubyte), n_values, file);
    // Close file
    fclose(file);
}

GLsizei DensityField::size() const {
    return m_res.x * m_res.y * m_res.z;
}

void DensityField::createTex() {
    // Use texture unit 0
    gl::ActiveTexture(gl::TEXTURE0 + TEX_U_DENS_V);
    // Allocate texture storage
    gl::GenTextures(1, &m_tex_handle);
    gl::BindTexture(gl::TEXTURE_3D, m_tex_handle);
    gl::TexStorage3D(gl::TEXTURE_3D, 1, gl::R8, m_res.x, m_res.y, m_res.z);
    // Use trilinear texture filering
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    // Use border-clamping for all 3 dimensions
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_BORDER);
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_BORDER);
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_WRAP_R, gl::CLAMP_TO_BORDER);
    // Upload the density data
    gl::TexSubImage3D(gl::TEXTURE_3D, 0, 0, 0, 0, m_res.x, m_res.y, m_res.z,
                      gl::RED, gl::UNSIGNED_BYTE, m_data);
}

void DensityField::computePiDensity(const PerspectiveCamera& cam, const Scene& scene) {
    printInfo("Performing fog density preintegration.");
    printInfo("This will take a few seconds. Please wait! :-)");
    const auto& res = m_pi_dens_res = ivec3{cam.resolution(), m_res.z};
    // Allocate storage
    m_pi_dens_data = new float[piDensSize()];
    // Set up render loop
    assert(0 == res.x % PACKET_SZ && 0 == res.y % PACKET_SZ);
    const ivec2 n_packets{cam.resolution() / PACKET_SZ};
    #pragma omp parallel for
    for (int p_j = 0; p_j < n_packets.y; ++p_j)
        for (int p_i = 0; p_i < n_packets.x; ++p_i)
            for (int p_y = 0; p_y < PACKET_SZ; ++p_y)
                for (int p_x = 0; p_x < PACKET_SZ; ++p_x) {
                    const int x{p_x + p_i * PACKET_SZ};
                    const int y{p_y + p_j * PACKET_SZ};
                    // Use pixel center: offset by 0.5
                    rt::Ray ray{cam.getPrimaryRay(x + 0.5f, y + 0.5f)};
                    // Intersect the bounding volume of density field
                    const auto is = m_bbox.intersect(ray);
                    if (is) {
                        // Determine distance to the geometry
                        scene.trace(ray);
                        // Compute parametric ray bounds
                        const float t_min{max(is.entr, 0.0f)};
                        const float t_max{min(is.exit, ray.inters.distance)};
                        // Sample density at interval endpoints
                        const int   n_intervals{res.z * 4};
                        const float dt{(t_max - t_min) / n_intervals};
                        // Perform ray marching
                        float prev_dens{sampleDensity(ray.o + t_min * ray.d)};
                        float dens{0.0f};
                        for (int i = 1; i <= n_intervals; ++i) {
                            // Distance to the end of the interval
                            const float t{t_min + i * dt};
                            const float curr_dens{sampleDensity(ray.o + t * ray.d)};
                            // Use trapezoidal rule for integration
                            dens += 0.5f * (curr_dens + prev_dens);
                            prev_dens = curr_dens;
                            if (2 == i % 4) {
                                // We are in the middle of the camera-space voxel (froxel)
                                const int z{i / 4};
                                m_pi_dens_data[x + y * res.x + z * res.x * res.y] = dens * dt;
                            }
                        }     
                    } else {
                        // Set density to zero along the ray
                        for (int z = 0; z < res.z; ++z) {
                            m_pi_dens_data[x + y * res.x + z * res.x * res.y] = 0.0f;
                        }
                    }
                }
    // Save it to disk
    writePiDens("Assets\\pi_df.3dt");
    // Load data into OpenGL texture
    createPiDensTex();
}

void DensityField::readPiDens(const char* const file_name) {
    // Open file
    auto file = fopen(file_name, "rb");
    if (!file) {
        // Something went wrong
        printError("Failed to open preintegrated density file %s for reading.", file_name);
        TERMINATE();
    }
    // Read resolution
    fread(&m_pi_dens_res, sizeof(GLsizei), 3, file);
    // Read image data
    const GLsizei n_values{piDensSize()};
    m_pi_dens_data = new GLfloat[n_values];
    fread(m_pi_dens_data, sizeof(GLfloat), n_values, file);
    // Close file
    fclose(file);
}

void DensityField::writePiDens(const char* const file_name) const {
    // Open file
    auto file = fopen(file_name, "wb");
    if (!file) {
        // Something went wrong
        printError("Failed to open preintegrated density file %s for writing.", file_name);
        TERMINATE();
    }
    // Write resolution
    fwrite(&m_pi_dens_res, sizeof(GLsizei), 3, file);
    // Write image data
    const GLsizei n_values{piDensSize()};
    fwrite(m_pi_dens_data, sizeof(GLfloat), n_values, file);
    // Close file
    fclose(file);
}

GLsizei DensityField::piDensSize() const {
    return m_pi_dens_res.x * m_pi_dens_res.y * m_pi_dens_res.z;
}

void DensityField::createPiDensTex() {
    // Use texture unit 0
    gl::ActiveTexture(gl::TEXTURE0 + TEX_U_PI_DENS);
    // Allocate texture storage
    gl::GenTextures(1, &m_pi_dens_tex_handle);
    gl::BindTexture(gl::TEXTURE_3D, m_pi_dens_tex_handle);
    gl::TexStorage3D(gl::TEXTURE_3D, 1, gl::R32F, m_pi_dens_res.x, m_pi_dens_res.y,
                     m_pi_dens_res.z);
    // Only linearly filter in Z-dimension
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    // Use edge-clamping for all 3 dimensions
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
    gl::TexParameteri(gl::TEXTURE_3D, gl::TEXTURE_WRAP_R, gl::CLAMP_TO_EDGE);
    // Upload the preintegrated density data
    gl::TexSubImage3D(gl::TEXTURE_3D, 0, 0, 0, 0, m_pi_dens_res.x, m_pi_dens_res.y,
                      m_pi_dens_res.z, gl::RED, gl::FLOAT, m_pi_dens_data);
}
