in vec3 fNormal;
in vec3 fPosition;

out vec3 gNormal;
out vec3 gPosition;

void main() {
    gNormal = fNormal;
    gPosition = fPosition;
}