
in vec3 position;
in vec3 normal;
in vec2 texcoord;

out vec3 fNormals;
out vec2 fTextureCoords;

void main() {
    gl_Position = vec4(position, 1.0);
    fNormals = normal;
    fTextureCoords = texcoord;
}