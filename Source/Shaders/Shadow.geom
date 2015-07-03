#version 440

// Vars IN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

layout (triangles) in;

layout (location = 1) uniform mat4 light_MVP[6];	// Model-view-projection matrix; locs 1..6
layout (location = 7) uniform int  layer_id;        // Layer index within cubemap array

// Vars OUT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

smooth out vec3 w_pos;                              // Vertex position in world coords

layout(triangle_strip, max_vertices = 18) out;      // 3 triangle vertices x 6 cubemap faces

// Implementation >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void main() {
    // Iterate over 6 cubemap faces
    for(int f = 0; f < 6; ++f) {
        for(int tri_vidx = 0; tri_vidx < 3; ++tri_vidx) {
            gl_Layer = layer_id + f;
            const vec4 w_pos4 = gl_in[tri_vidx].gl_Position;
            w_pos = w_pos4.xyz;
            gl_Position = light_MVP[f] * w_pos4;
            EmitVertex();
        }
        EndPrimitive();
    }
}
