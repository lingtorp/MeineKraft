#ifdef FLAG_CUBE_MAP_TEXTURE
flat in int fDiffuse_texture_idx;
uniform samplerCubeArray diffuse_sampler;
#endif
#ifdef FLAG_2D_TEXTURE
flat in int fDiffuse_texture_idx;
uniform sampler2D diffuse_sampler;
#endif

in vec4 fColor;    // This name must match the name in the vertex shader in order to work
in vec2 fTexcoord; // passthrough shading for interpolated textures
in vec3 fNormal;   // Normalized world space interpolated surface normal
in vec4 fPosition; // Model position in world space
in vec4 fNonModelPos; // Local space position, needed by cubeSampler

out vec4 outColor; // Defaults to zero when the frag shader only has 1 out variable

/// Lights
struct Light {
    vec4 color;
    vec4 light_intensity; // (ambient, diffuse, specular, padding) - itensities
    vec3 position;
};

const int MAX_NUM_LIGHTS = 1;

layout (std140) uniform lights_block {
    Light lights[MAX_NUM_LIGHTS];
};

uniform vec3 camera_position; // Position of the eye/camera

// View or a.k.a camera matrix
uniform mat4 camera_view;

// Projection or a.k.a perspective matrix
uniform mat4 projection;

#ifdef FLAG_SSAO
uniform sampler2D ssao_sampler;
#endif

void main() {
    outColor = vec4(1.0); // Sets a default color of white to all objects
    vec4 default_light = vec4(1.0, 1.0, 1.0, 1.0);

    /// SSAO
    float ambient_occlusion = 0.0;
#ifdef FLAG_SSAO
    ambient_occlusion = texture(ssao_sampler, gl_FragCoord.xy);
#endif

#ifdef FLAG_BLINN_PHONG_SHADING
    vec3 total_light = vec3(0.0, 0.0, 0.0);
    vec3 eye = normalize(camera_position - fPosition.xyz);
    float ambient_light;

    for (int i = 0; i < MAX_NUM_LIGHTS; i++) {
        Light light = lights[i];
        float ambient_intensity  = light.light_intensity.x;
        float diffuse_intensity  = light.light_intensity.y;
        float specular_intensity = light.light_intensity.z;
        vec3 direction = normalize(lights[i].position.xyz - fPosition.xyz);

        vec3 diffuse_light = light.color.xyz * max(dot(fNormal, direction), 0.0) * diffuse_intensity;

        vec3 reflection = 2 * dot(direction, fNormal) * fNormal - direction;
        vec3 specular_light = vec3(dot(reflection, normalize(eye))) * specular_intensity;

        total_light += diffuse_light;
        total_light += specular_light;
        total_light += ambient_intensity;
    }

   default_light = vec4(clamp(total_light, 0.0, 1.0), 1.0);
   outColor = default_light;
#endif

#ifdef FLAG_2D_TEXTURE
    outColor = texture(diffuse_sampler, vec3(fTexcoord, fDiffuse_texture_idx)) * default_light;
#endif
#ifdef FLAG_CUBE_MAP_TEXTURE
    outColor = texture(diffuse_sampler, vec4(normalize(fNonModelPos.xyz), fDiffuse_texture_idx)) * default_light;
#endif

    vec2 frag_coord = vec2(gl_FragCoord.x / 1280.0, gl_FragCoord.y / 720.0);
    float origin_depth = linearize_depth(frag_coord);
    // outColor = vec4(vec3(origin_depth), 1.0);
    // vec3 tex_normal = texture(normal_sampler, frag_coord).xyz;
    // outColor = vec4(tex_normal, 1.0);
    // outColor = vec4(fNormal, 1.0);
    outColor = vec4(vec3(ambient_occlusion), 1.0);
    // outColor = vec4(rvec, 1.0);
    // outColor = vec4(vec3(ssao_samples[0]), 1.0);
}
