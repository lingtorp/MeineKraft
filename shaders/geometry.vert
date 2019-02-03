
uniform mat4 projection;
uniform mat4 camera_view;

in uint instance_idx; 
in vec3 position;
in vec3 normal;
in vec2 texcoord;

layout(std140, binding = 2) readonly buffer ModelsBlock {
    mat4 models[];
};

out vec3 fNormal;
out vec3 fPosition;
out vec2 fTexcoord;

#ifdef DIFFUSE_CUBEMAP
out vec3 local_space_position;
#endif

flat out uint fInstance_idx;

void main() {
    gl_Position = projection * camera_view * models[instance_idx] * vec4(position, 1.0);

    fNormal = normal;
    fPosition = vec3(models[instance_idx] * vec4(position, 1.0));
    fTexcoord = texcoord;
    fInstance_idx = instance_idx;
    #ifdef DIFFUSE_CUBEMAP
    local_space_position = position;
    #endif
}
