
in vec3 position;
in vec3 normal;
in vec2 texcoord;

out VS_OUT {
    out vec3 gsNormal;
    out vec2 gsTextureCoord;
    out vec3 gsPosition;
} vs_out;

void main() {
    gl_Position = vec4(position, 1.0);
    vs_out.gsNormal = normal;
    vs_out.gsTextureCoord = texcoord;
    vs_out.gsPosition = position;
}
