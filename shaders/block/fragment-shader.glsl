#version 330 core   // tex.coords (s, t, r) == (x, y, z)

in  vec4 fColor; // This name must match the name in the vertex shader in order to work
out vec4 outColor;

in vec2 fTexcoord;   // passthrough shading for interpolated textures
uniform sampler2D diffuse_sampler;

in vec3 fNormal;
in vec4 fPosition;

/// Lights
struct Light {
    vec4 color;
    vec4 light_intensity; // (ambient, diffuse, specular, padding) itensity
    vec3 position;
};

const int MAX_NUM_LIGHTS = 1;

layout (std140) uniform lights_block {
//    uint number_lights;
    Light lights[MAX_NUM_LIGHTS];
};

uniform vec3 camera_position; // Position of the eye/camera

void main() {
    vec3 total_light = vec3(0.0, 0.0, 0.0);
    vec3 normal = normalize(fNormal);
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
}
