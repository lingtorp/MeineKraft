
in vec3 fNormal;
in vec3 fPosition;
in vec2 fTexcoord;
flat in int fDiffuse_layer_idx;
flat in int fShading_model_id;
flat in vec3 fPbr_scalar_parameters;

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
    
    #ifdef PBR_SCALAR
    // gDiffuse texture is usually in sRGB
    gDiffuse.rgb = pow(vec3(1.0, 0.0, 0.0), vec3(1.0 / 2.2)); // FIXME: Temporary, diffuse scalar colors as a shader config?
    // Usually (metallic, roughness, unused)
    gPBRParameters = fPbr_scalar_parameters;
    gEmissive = vec3(0.0);
    #elif defined(PBR_TEXTURED)
    gPBRParameters = texture(pbr_parameters, fTexcoord).rgb;
    gEmissive = texture(emissive, fTexcoord).rgb;
    #endif
    
    gAmbientOcclusion = texture(ambient_occlusion, fTexcoord).rgb;
    gShadingModelID = fShading_model_id;
}