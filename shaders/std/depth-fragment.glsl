in vec3 fNormal;
in vec3 fPosition;
in vec2 fTexcoord;

out vec3 gNormal;
out vec3 gPosition;
out vec4 gDiffuse;

uniform sampler2DArray diffuse;

void main() {
    gNormal = fNormal;
    gPosition = fPosition;
    gDiffuse.rgb = texture(diffuse, vec3(fTexcoord, 0)).rgb;
    gDiffuse.a = 0.5;
}