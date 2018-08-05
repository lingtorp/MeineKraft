in vec3 fNormal;
in vec3 fPosition;
in vec2 fTexcoord;

out vec3 gNormal;
out vec3 gPosition;
out vec4 gDiffuse;

uniform sampler2DArray diffuse;
uniform sampler2DArray specular;

void main() {
    gNormal = normalize(fNormal);
    gPosition = fPosition;
    gDiffuse.rgb = vec3(1.0); //texture(diffuse, vec3(fTexcoord, 0)).rgb;
    gDiffuse.a = 1.0; // texture(specular, vec3(fTexcoord, 0)).r;
}