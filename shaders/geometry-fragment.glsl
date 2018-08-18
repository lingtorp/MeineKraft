in vec3 fNormal;
in vec3 fPosition;
in vec2 fTexcoord;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out vec3 gPosition;
layout(location = 2) out vec4 gDiffuse;

//uniform sampler2DArray diffuse;
//uniform sampler2DArray specular;

void main() {
    gNormal = normalize(fNormal);
    gPosition = fPosition;
    gDiffuse.rgb = vec3(fTexcoord, 0.0); //texture(diffuse, vec3(fTexcoord, 0)).rgb;
    gDiffuse.a = 1.0; // texture(specular, vec3(fTexcoord, 0)).r;
}