#version 440

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (location = 0) in vec3 vert_m_pos;   // Vertex position in model coordinates
layout (location = 1) in vec3 vert_m_norm;  // Vertex normal in model coordinates

uniform mat4 model_mat;                     // Model-to-world space transformation matrix
uniform mat4 MVP;                           // Model-to-clip (homogeneous) space transformation matrix
uniform mat3 norm_mat;                      // Model-to-camera space normal vector transformation matrix

// Vars OUT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

smooth out vec3 w_pos;                      // Position in world space
smooth out vec3 w_norm;                     // Normal in camera space

// Implementation >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void main() {
    const vec4 vert_m_pos4 = vec4(vert_m_pos, 1.0);
    // Transform vertex position and normal to world space
    w_pos  = (model_mat * vert_m_pos4).xyz;
    w_norm = normalize(norm_mat * vert_m_norm);
    // Transform vertex position to clip coordinates
    gl_Position = MVP * vert_m_pos4;
}
