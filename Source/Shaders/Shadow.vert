#version 440

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (location = 0) in vec3 vert_m_pos;		// Vertex position in model coords

layout (location = 0) uniform mat4 model_mat;	// Model to world coords transformation matrix

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void main() {
    // Transform model coordinates to world coordinates
    gl_Position = model_mat * vec4(vert_m_pos, 1.0);
}
