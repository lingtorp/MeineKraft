in  vec4 fColor; // This name must match the name in the vertex shader in order to work
out vec4 outColor;

in vec2 fTexcoord;   // passthrough shading for interpolated textures
#ifdef CUBE_MAP_TEXTURE
uniform samplerCube diffuse_sampler;
#endif
#ifndef CUBE_MAP_TEXTURE
uniform sampler2D diffuse_sampler;
#endif

in vec3 fNormal;
in vec4 fPosition;

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

void main() {
#ifdef BLINN_PHONG_SHADING
    vec3 total_light = vec3(0.0, 0.0, 0.0);
    vec3 normal = fNormal; // already normalized
    vec3 eye = normalize(camera_position - fPosition.xyz);
    float ambient_light;

    for (int i = 0; i < MAX_NUM_LIGHTS; i++) {
        Light light = lights[i];
        float ambient_intensity = light.light_intensity.x;
        float diffuse_intensity = light.light_intensity.y;
        float specular_intensity = light.light_intensity.z;
        vec3 direction = normalize(lights[i].position.xyz - fPosition.xyz);

        vec3 diffuse_light = light.color.xyz * max(dot(normal, direction), 0.0) * diffuse_intensity;

        vec3 reflection = 2*dot(direction, normal)*normal - direction;
        vec3 specular_light = vec3(dot(reflection, normalize(eye))) * specular_intensity;

        total_light += diffuse_light;
        total_light += specular_light;
        total_light += vec3(ambient_intensity);
    }

   total_light = clamp(total_light, 0.0, 1.0);
   outColor = texture(diffuse_sampler, fTexcoord) * vec4(total_light, 1.0);
#endif
#ifdef CUBE_MAP_TEXTURE
    outColor = texture(diffuse_sampler, normalize(fPosition.xyz));
#endif
}
