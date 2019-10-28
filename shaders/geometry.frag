// #version 450 core

in vec3 fTangent;
in vec3 fGeometricNormal;
in vec3 fPosition;
in vec2 fTexcoord;
flat in uint fInstance_idx; 

layout(location = 0) out vec3 gGeometricNormal;
layout(location = 1) out vec3 gPosition;
layout(location = 2) out vec4 gDiffuse;
layout(location = 3) out vec3 gPBRParameters;
layout(location = 4) out vec3 gEmissive;
layout(location = 5) out uint gShadingModelID;
layout(location = 6) out vec3 gTangentNormal;
layout(location = 7) out vec3 gTangent;

#ifdef DIFFUSE_2D
uniform sampler2DArray diffuse;
#elif defined(DIFFUSE_CUBEMAP)
in vec3 local_space_position; // Used to sample cubemap
uniform samplerCubeArray diffuse;
#endif
uniform sampler2D pbr_parameters;
uniform sampler2D emissive;
uniform sampler2D tangent_normal;

struct Material {
  uint diffuse_layer_idx;
  uint shading_model;
  vec2 pbr_scalar_parameters; // (roughness, metallic)
};

layout(std140, binding = 3) readonly buffer MaterialBlock {
    Material materials[];
};

void main() {
    const Material material = materials[fInstance_idx];

    gTangent = fTangent;
    gGeometricNormal = normalize(fGeometricNormal);
    gTangentNormal = texture(tangent_normal, fTexcoord).xyz;
    gPosition = fPosition;
    
    #ifdef DIFFUSE_2D
    
    #ifdef DIFFUSE_RGB
    gDiffuse.rgb = texture(diffuse, vec3(fTexcoord, material.diffuse_layer_idx)).rgb; 
    gDiffuse.a = 1.0;
    #else 
    gDiffuse.rgba = texture(diffuse, vec3(fTexcoord, material.diffuse_layer_idx)).rgba; 
    #endif
    
    #elif defined(DIFFUSE_CUBEMAP)
    gDiffuse.rgb = texture(diffuse, vec4(local_space_position, material.diffuse_layer_idx)).rgb;
    gDiffuse.a = 1.0;
    #endif

    switch (material.shading_model) {
        case 2: // Physically based rendering with textures 
        gPBRParameters = texture(pbr_parameters, fTexcoord).rgb; // Usually (unused, metallic, roughness)
        #if defined(HAS_EMISSIVE_TEXTURE)
        gEmissive = texture(emissive, fTexcoord).rgb;
        #else
        gEmissive = vec3(0.0);
        #endif 
        break;
        case 3: // Physically based rendering with scalar
        gDiffuse.rgba = vec4(1.0, 0.0, 0.0, 1.0); // FIXME: Temporary color for unlit objects
        gPBRParameters = vec3(0.0, material.pbr_scalar_parameters); // FIXME: Redefine pbr parameteres ...
        gEmissive = vec3(0.0);
        break;
    }

    gShadingModelID = material.shading_model;
}
