
uniform mat4 light_space_transform; // projection * camera_view (for the light)

in uint instance_idx; 
in vec3 position;

layout(std140, binding = 2) readonly buffer ModelsBlock {
    mat4 models[];
};

void main() {
    gl_Position = light_space_transform * models[instance_idx] * vec4(position, 1.0);
}