
uniform mat4 projection;
uniform mat4 camera_view;

in vec3 position;
in vec3 normal;
in vec2 texcoord;
in vec3 tangent;
in uint instance_idx; 

layout(std140, binding = 2) readonly buffer ModelsBlock {
    mat4 models[];
};

out vec3 fTangent;
out vec3 fGeometricNormal;
out vec3 fPosition;
out vec2 fTexcoord;

#ifdef DIFFUSE_CUBEMAP
out vec3 local_space_position;
#endif

flat out uint fInstance_idx;

void main() {
    const vec4 p = models[instance_idx] * vec4(position, 1.0); 
    gl_Position = camera_view * p;
    fTangent = tangent;
    fGeometricNormal = normal;
    fPosition = p.xyz;
    fTexcoord = texcoord;
    fInstance_idx = instance_idx;
    #ifdef DIFFUSE_CUBEMAP
    local_space_position = position;
    #endif
}
