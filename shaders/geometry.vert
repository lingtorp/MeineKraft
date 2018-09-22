#version 410 core 

uniform mat4 projection;
uniform mat4 camera_view;

in mat4 model;
in vec3 position;
in vec3 normal;
in vec2 texcoord;
in int diffuse_layer_idx;

out vec3 fNormal;
out vec3 fPosition;
out vec2 fTexcoord;
flat out int fDiffuse_layer_idx;

void main() {
    gl_Position = projection * camera_view * model * vec4(position, 1.0);

    mat3 normal_matrix = mat3(camera_view * model);
    fNormal = normal_matrix * normal;
    fPosition = vec3(model * vec4(position, 1.0));
    fTexcoord = texcoord;
    fDiffuse_layer_idx = diffuse_layer_idx;
}