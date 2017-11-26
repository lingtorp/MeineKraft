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

// Depth texture
uniform sampler2D depth_sampler;

// SSAO Kernel samples
const int NUM_SSAO_SAMPLES = 16;
uniform vec3 ssao_samples[16];
uniform sampler2D noise_sampler;
const vec2 noise_scale = vec2(1280.0 / 4.0, 720.0 / 4.0);

float linearize_depth(vec2 uv) {
  float n = 1.0;  // camera z near
  float f = 10.0; // camera z far
  float z = texture(depth_sampler, uv).r;
  return (2.0 * n) / (f + n - z * (f - n));
}

void main() {
    outColor = vec4(1.0f); // Sets a default color of white to all objects
    vec4 default_light = vec4(1.0, 1.0, 1.0, 1.0);

    /// SSAO
    float occlusion = 0.0;
// #ifdef FLAG_SSAO
    vec3 origin = fPosition.xyz;
    const float kernel_radius = 1.0;

    // Orientate kernel sample hemisphere
    vec3 rvec = texture(noise_sampler, fTexcoord * noise_scale).xyz * 2.0 - 1.0;
    vec3 tangent = normalize(rvec - fNormal * dot(rvec, fNormal));
    vec3 bitangent = cross(fNormal, tangent);
    mat3 tbn = mat3(tangent, bitangent, fNormal);

    for (int i = 0; i < NUM_SSAO_SAMPLES; i++) {
        // Get sample position
        vec3 kernel_sample = tbn * ssao_samples[i];
        kernel_sample = origin + kernel_sample * kernel_radius;
        // Project sample position
        vec4 proj_sample = vec4(kernel_sample, 1.0);
        proj_sample = projection * proj_sample;
        proj_sample.xy /= proj_sample.w;
        proj_sample.xy = proj_sample.xy * 0.5 + 0.5;
        // Get samples depth
        float sample_depth = linearize_depth(proj_sample.xy);
        // Check for occlusion
        float in_range = abs(origin.z - sample_depth) < kernel_radius ? 1.0 : 0.0;
        occlusion += (sample_depth <= proj_sample.z ? 1.0 : 0.0) * in_range;
    }
    occlusion = 1.0 - (occlusion / float(NUM_SSAO_SAMPLES));
// #endif

#ifdef FLAG_BLINN_PHONG_SHADING
    vec3 total_light = vec3(0.0, 0.0, 0.0);
    vec3 normal = fNormal; // already normalized
    vec3 eye = normalize(camera_position - fPosition.xyz);
    float ambient_light;

    for (int i = 0; i < MAX_NUM_LIGHTS; i++) {
        Light light = lights[i];
        float ambient_intensity  = light.light_intensity.x;
        float diffuse_intensity  = light.light_intensity.y;
        float specular_intensity = light.light_intensity.z;
        vec3 direction = normalize(lights[i].position.xyz - fPosition.xyz);

        vec3 diffuse_light = light.color.xyz * max(dot(normal, direction), 0.0) * diffuse_intensity;

        vec3 reflection = 2 * dot(direction, normal) * normal - direction;
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
    // TODO: Screen dimensions as uniforms
    // vec2 sample_pos = vec2(gl_FragCoord.x / 1280.0, gl_FragCoord.y / 720.0);
    // outColor = vec4(vec3(linearize_depth(sample_pos)), 1.0f);
    outColor = vec4(vec3(occlusion), 1.0);
}
