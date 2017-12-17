in vec3 fNormal;

out vec3 gNormal;

void main() {
    gNormal = fNormal * 0.5 + 0.5;
}