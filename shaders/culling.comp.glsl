// Required by compute shaders apparently
// Process one object per shader invocation (optimisation?)
layout (local_size_x = 1, local_size_y = 1) in;

layout(std140, binding = 3) readonly buffer BoundingVolumeBlock {
    vec4 spheres[]; // vec4 = (center.xyz, radius)
};

// Plane defined as: Ax + By + Cz = D
uniform vec4 frustum_planes[6];

uint test(vec4 obj, vec4 plane) {
    const float distance = plane.x * obj.x + plane.y * obj.y + plane.z * obj.z + plane.w;
    if (distance < -obj.w) {
        return 0; // Negative halfspace
    }
    return 1; // Positive halfspace or on the plane
}

/// Same as the OpenGL provided struct: DrawElementsIndirectCommand
struct DrawCommand {
    uint count; // Num elements (vertices)
    uint instanceCount; // Number of instances to draw (a.k.a primcount)
    uint firstIndex; // Specifies a byte offset (cast to a pointer type) into the buffer bound to GL_ELEMENT_ARRAY_BUFFER to start reading indices from.
    uint baseVertex; // Specifies a constant that should be added to each element of indices​ when chosing elements from the enabled vertex arrays.
    uint baseInstance; // Specifies the base instance for use in fetching instanced vertex attributes.
    // 20 bytes
    uint padding0;
    uint padding1;
    uint padding2;
    // 32 bytes (multiple of 16)
};

// One draw command per object 
// Command buffer backed by Shader Storage Object Buffer (SSBO)
layout(std140, binding = 0) writeonly buffer DrawCommandsBlock {
    DrawCommand draw_commands[];
};

// Model (MVP) buffer backed by Shader Storage Object Buffer (SSBO) in the draw shader
layout(std140, binding = 1) writeonly buffer MVPBlock {
    mat4 MVPs[];
};

// Projection-view matrix
uniform mat4 proj_view;

// Number of items to be processed 
uniform uint NUM_ITEMS;

void main() {
    const uint idx = gl_GlobalInvocationID.y * gl_NumWorkGroups.x + gl_GlobalInvocationID.x; 
    if (idx >= NUM_ITEMS) { return; }

    uint inside = 0; 
    for (int i = 0; i < 6; i++) {
        inside += test(spheres[idx], frustum_planes[i]) << i;
    }

    const uint INSIDE_ALL_PLANES = 63; // = 0b111111;
    const bool visible = inside == INSIDE_ALL_PLANES;
    draw_commands[idx].count = 25350; // sphere.indices.size();
    draw_commands[idx].instanceCount = visible ? 1 : 0; // This is the trick right here
    draw_commands[idx].baseInstance = 0;
    draw_commands[idx].baseVertex = 0;
    draw_commands[idx].padding0 = 0; // Avoid optimisation 
    draw_commands[idx].padding1 = 0;
    draw_commands[idx].padding2 = 0;

    // Calculate MVP
    // Perform translation using the bounding volume's position
    mat4 model = mat4(0.0);
    model[0][0] = 1.0;
    model[1][1] = 1.0;
    model[2][2] = 1.0;
    model[3][3] = 1.0;
    model[3][0] = spheres[idx].x;
    model[3][1] = spheres[idx].y;
    model[3][2] = spheres[idx].z;
    MVPs[idx] = proj_view * model;
}