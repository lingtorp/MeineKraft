
uniform mat4 projection;
uniform mat4 camera_view;

in mat4 model;
in vec3 position;
in vec3 normal;
in vec2 texcoord;
in int diffuse_layer_idx;
in int shading_model_id;

out vec3 fNormal;
out vec3 fPosition;
out vec2 fTexcoord;
flat out int fDiffuse_layer_idx;
flat out int fShading_model_id;

void main() {
    gl_Position = projection * camera_view * model * vec4(position, 1.0);

    mat3 normal_matrix = mat3(camera_view * model);
    fNormal = normal_matrix * normal;
    #ifdef DIFFUSE_2D
    fPosition = vec3(model * vec4(position, 1.0));
    #elif defined(DIFFUSE_CUBEMAP)
    fPosition = vec3(vec4(position, 1.0));
    #endif
    fTexcoord = texcoord;
    fDiffuse_layer_idx = diffuse_layer_idx;
    fShading_model_id = shading_model_id;
}