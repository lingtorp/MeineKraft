// Required by compute shaders apparently
// Process one object per shader invocation (optimisation?)
layout (local_size_x = 1) in;

// TODO: Turn into SSBO
const uint MAX_OBJECTS = 1000;
uniform vec4 spheres[MAX_OBJECTS]; // vec4 = (center.xyz, radius)

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
    uint count;         // Num elements (vertices)
    uint instanceCount; // Number of instances to draw (a.k.a primcount)
    uint firstIndex;    // Specifies a byte offset (cast to a pointer type) into the buffer bound to GL_ELEMENT_ARRAY_BUFFER to start reading indices from.
    uint baseVertex;    // Specifies a constant that should be added to each element of indices​ when chosing elements from the enabled vertex arrays.
    uint baseInstance;  // Specifies the base instance for use in fetching instanced vertex attributes.
    // 20 bytes
    uint padding0;
    uint padding1;
    uint padding2;
    // 32 bytes (multiple of 16)
};

// Command buffer backed by Shader Storage Object Buffer (SSBO)
layout(std140, binding = 0) buffer DrawCommandsBlock {
    DrawCommand draw_commands[];
};

// Index of the currently created draw command (into draw_commands)
uniform uint DRAW_CMD_IDX = 0; // Will be used in the future when all multi draw cmds are one

// Object index which gives shader data later in the pipeline 
// Note: Using std430 to suppress 16 byte aligned writes 
layout(std430, binding = 1) writeonly buffer ShaderDataIndexBlock {
    uint index_buffer[];
};

// Number of indices (a.k.a elements) of the currently created draw command (into draw_commands)
uniform uint NUM_INDICES;

void main() {
    const uint idx = gl_GlobalInvocationID.y * gl_NumWorkGroups.x + gl_GlobalInvocationID.x; 

    uint inside = 0; 
    for (int i = 0; i < 6; i++) {
        inside += test(spheres[idx], frustum_planes[i]) << i;
    }

    const uint INSIDE_ALL_PLANES = 63; // = 0b111111;
    const bool visible = inside == INSIDE_ALL_PLANES;
    if (visible) {
        draw_commands[DRAW_CMD_IDX].count = NUM_INDICES;
        draw_commands[DRAW_CMD_IDX].baseInstance = 0;
        draw_commands[DRAW_CMD_IDX].baseVertex = 0;
        draw_commands[DRAW_CMD_IDX].padding0 = 0; // Avoid optimisation 
        draw_commands[DRAW_CMD_IDX].padding1 = 0;
        draw_commands[DRAW_CMD_IDX].padding2 = 0;

        // 

        const uint INSTANCE_IDX = atomicAdd(draw_commands[DRAW_CMD_IDX].instanceCount, 1);
        index_buffer[INSTANCE_IDX] = idx; // Shader data index (idx) for objects
    }
}