#version 410 core 

in vec3 fNormal;
in vec3 fPosition;
in vec2 fTexcoord;
flat in int fDiffuse_layer_idx;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out vec3 gPosition;
layout(location = 2) out vec4 gDiffuse;
layout(location = 3) out vec3 gPBRParameters;
layout(location = 4) out vec3 gAmbientOcclusion;

uniform sampler2DArray diffuse;
uniform sampler2D pbr_parameters;
uniform sampler2D ambient_occlusion;

void main() {
    gNormal = normalize(fNormal);
    gPosition = fPosition;
    gDiffuse.rgb = texture(diffuse, vec3(fTexcoord, fDiffuse_layer_idx)).rgb; 
    gDiffuse.a = 1.0; // Fetch from texture?
    gPBRParameters = texture(pbr_parameters, fTexcoord).rgb;
    gAmbientOcclusion = texture(ambient_occlusion, fTexcoord).rgb;
}