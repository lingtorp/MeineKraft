#ifdef FLAG_CUBE_MAP_TEXTURE
uniform samplerCube diffuse_sampler;
#endif
#ifdef FLAG_2D_TEXTURE
uniform sampler2D diffuse_sampler;
#endif

in vec4 fColor;    // This name must match the name in the vertex shader in order to work
in vec2 fTexcoord; // passthrough shading for interpolated textures
in vec3 fNormal;
in vec4 fPosition;

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

#define CRYTEK_AO
#define M_PI 3.1415926535897932384626433832795

// Given a z-buffer value it linearizes it depending on the n (near plane) and f (far plane) of the view frustrum
float LinearizeDepth(float z, float n, float f) {
    float depth = (2.0 * n) / (f + n - z * (f - n));  // convert to linear values
    return depth;
}

void main() {
    outColor = vec4(1.0f); // Sets a default color of white to all objects
    vec4 default_light = vec4(1.0, 1.0, 1.0, 1.0);
    float occlusion_factor = 0.0f;

    // AO pass
#ifndef FLAG_2D_TEXTURE
    float z = gl_FragCoord.z; // depth-buffer value for the current pixel
    int occluding_points = 0;
    #ifdef CRYTEK_AO
        const int NUM_SAMPLES = 10 * 4;
        float R = 1.0f;
        const float[10] thetas = float[](0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f);
        const float[4]  fis    = float[](0.25f, 0.5f, 0.75f, 1.0f);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 10; j++) {
                // Generate sample point in world space
                float f = fis[i];
                float t = thetas[j];
                vec4 sample_point = vec4(R * cos(2 * M_PI * t) * sin(M_PI * f) + fPosition.x,
                                         R * sin(2 * M_PI * t) * sin(M_PI * f) + fPosition.y,
                                         R * sin(M_PI * f) + fPosition.z,
                                         1.0f);
                // Transform sample point from view space to screen space to get its depth value
                sample_point = projection * camera_view * sample_point; // Clip space
                sample_point = sample_point / sample_point.w;           // Perspective division - Normalized device coordinate
                float sample_depth = 0.5f * sample_point.z + 0.5f;      // Viewport transform for z - window space

                // Check whether sample_point is behind current pixel depth
                if (z < sample_depth) { occluding_points++; }
            }
        }
        occlusion_factor = float(occluding_points) / float(10.0f * 4.0f);
    #endif
    // outColor = vec4(occlusion_factor);
    outColor = vec4(1.0f - occlusion_factor); // Ambient occlusion
#endif

#ifndef FLAG_BLINN_PHONG_SHADING
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
        // total_light += specular_light;
        total_light += vec3(ambient_intensity) * (1.0f - occlusion_factor);
    }

   default_light = vec4(clamp(total_light, 0.0, 1.0), 1.0);
   outColor = default_light;
#endif

#ifdef FLAG_2D_TEXTURE
    outColor = texture(diffuse_sampler, fTexcoord) * default_light;
#endif
#ifdef FLAG_CUBE_MAP_TEXTURE
    outColor = texture(diffuse_sampler, normalize(fPosition.xyz)) * default_light;
#endif
}
