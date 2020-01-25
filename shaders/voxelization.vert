in vec3 position;
in vec3 normal;
in vec2 texcoord;
in uint instance_idx;

layout(std140, binding = 2) readonly buffer ModelsBlock {
  mat4 models[];
};

out VS_OUT {
    out vec3 gsNormal;
    out vec2 gsTextureCoord;
    out vec3 gsPosition;
    flat out uint gsInstanceIdx;
} vs_out;

void main() {
    const vec4 p = models[instance_idx] * vec4(position, 1.0);
    gl_Position = p;
    vs_out.gsNormal = normal;
    vs_out.gsTextureCoord = texcoord;
    vs_out.gsPosition = p.xyz;
    vs_out.gsInstanceIdx = instance_idx;
}
