
in vec3 fNormal;
in vec3 fPosition;
in vec2 fTexcoord;
flat in int fDiffuse_layer_idx;
flat in int fShading_model_id;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out vec3 gPosition;
layout(location = 2) out vec4 gDiffuse;
layout(location = 3) out vec3 gPBRParameters;
layout(location = 4) out vec3 gAmbientOcclusion;
layout(location = 5) out vec3 gEmissive;
layout(location = 6) out int  gShadingModelID;

#ifdef DIFFUSE_2D
uniform sampler2DArray diffuse;
#elif defined(DIFFUSE_CUBEMAP)
uniform samplerCubeArray diffuse;
#endif
uniform sampler2D pbr_parameters;
uniform sampler2D ambient_occlusion;
uniform sampler2D emissive;

void main() {
    gNormal = normalize(fNormal);
    gPosition = fPosition;
    #ifdef DIFFUSE_2D
    gDiffuse.rgb = texture(diffuse, vec3(fTexcoord, fDiffuse_layer_idx)).rgb; 
    #elif defined(DIFFUSE_CUBEMAP)
    gDiffuse.rgb = texture(diffuse, vec4(normalize(fPosition), fDiffuse_layer_idx)).rgb;
    #endif
    gDiffuse.a = 1.0; // Fetch from texture?
    gPBRParameters = texture(pbr_parameters, fTexcoord).rgb;
    gAmbientOcclusion = texture(ambient_occlusion, fTexcoord).rgb;
    gEmissive = texture(emissive, fTexcoord).rgb;
    gShadingModelID = fShading_model_id;
}