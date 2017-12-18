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

uniform sampler2D depth_sampler;
uniform sampler2D normal_sampler;

// SSAO Kernel samples
uniform int NUM_SSAO_SAMPLES;
uniform vec3 ssao_samples[512];
uniform sampler2D noise_sampler;
const vec2 noise_scale = vec2(1280.0 / 8.0, 720.0 / 8.0);
uniform float ssao_kernel_radius;

float linearize_depth(vec2 uv) {
  float n = 1.0;  // camera z near
  float f = 10.0; // camera z far
  float z = texture(depth_sampler, uv).r;
  return (2.0 * n) / (f + n - z * (f - n));
}

float linearize_depth(float z) {
  float n = 1.0;  // camera z near
  float f = 10.0; // camera z far
  return (2.0 * n) / (f + n - z * (f - n));
}

void main() {
    outColor = vec4(1.0); // Sets a default color of white to all objects
    vec4 default_light = vec4(1.0, 1.0, 1.0, 1.0);

    /// SSAO
    float ambient_occlusion = 0.0;

    // Orientate kernel sample hemisphere
    vec3 rvec = texture(noise_sampler, gl_FragCoord.xy * noise_scale).xyz;
    vec3 tangent = normalize(rvec - fNormal * dot(rvec, fNormal));
    vec3 bitangent = cross(fNormal, tangent);
    mat3 tbn = mat3(tangent, bitangent, fNormal); // World space to tangent space (tilted world space ... )

    for (int i = 0; i < NUM_SSAO_SAMPLES; i++) {
        // 1. Get sample point
        vec4 point = vec4(vec3(fPosition.xyz + tbn * ssao_samples[i] * ssao_kernel_radius), 1.0);
        // 2. Project the sample
        point = projection * camera_view * point;
        point.xy /= point.w;
        point.xy = point.xy * 0.5 + 0.5;
        // 3. Lookup the sample's real depth
        float point_depth = linearize_depth(point.xy);
        // 4. Compare depths
        if (point_depth < point.z) { ambient_occlusion += 1.0; }
    }
    ambient_occlusion = 1.0 - (ambient_occlusion / float(NUM_SSAO_SAMPLES));

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
