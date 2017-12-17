in vec4 fPosition;
in vec3 fNormal;

out vec4 gPosition;
out vec3 gNormal;

void main() {
    gPosition = fPosition;
    gNormal = fNormal;
}