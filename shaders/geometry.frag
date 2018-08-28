#version 410 core 

in vec3 fNormal;
in vec3 fPosition;
in vec2 fTexcoord;
flat in int fDiffuse_layer_idx;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out vec3 gPosition;
layout(location = 2) out vec4 gDiffuse;

uniform samplerCubeArray diffuse;

void main() {
    gNormal = normalize(fNormal);
    gPosition = fPosition;
    gDiffuse.rgb = texture(diffuse, vec4(normalize(fPosition), fDiffuse_layer_idx)).rgb; 
    // gDiffuse.rgb = vec3(fTexcoord, float(fDiffuse_layer_idx)).rgb; 
    // gDiffuse.rgb = texture(diffuse, vec3(fTexcoord, fDiffuse_layer_idx)).rgb;
    gDiffuse.a = 1.0; 
}