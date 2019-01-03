
uniform mat4 projection;
uniform mat4 camera_view;

in uint instance_idx; 
in vec3 position;
in vec3 normal;
in vec2 texcoord;

layout(std140) readonly buffer ModelsBlock {
    mat4 models[];
};

in int diffuse_layer_idx;
in int shading_model_id;
in vec3 pbr_scalar_parameters;

out vec3 fNormal;
out vec3 fPosition;
out vec2 fTexcoord;
flat out int fDiffuse_layer_idx;
flat out int fShading_model_id;
flat out vec3 fPbr_scalar_parameters;

void main() {
    gl_Position = projection * camera_view * models[instance_idx] * vec4(position, 1.0);

    fNormal = normal;
    fPosition = vec3(models[instance_idx] * vec4(position, 1.0));
    #if defined(DIFFUSE_CUBEMAP)
    fPosition = vec3(vec4(position, 1.0));
    #endif
    fTexcoord = texcoord;
    fDiffuse_layer_idx = diffuse_layer_idx;
    fShading_model_id = shading_model_id;
    fPbr_scalar_parameters = pbr_scalar_parameters;
}
