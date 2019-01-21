
uniform mat4 projection;
uniform mat4 camera_view;

in uint instance_idx; 
in vec3 position;

layout(std140, binding = 2) readonly buffer ModelsBlock {
    mat4 models[];
};

void main() {
    gl_Position = projection * camera_view * models[instance_idx] * vec4(position, 1.0);
}