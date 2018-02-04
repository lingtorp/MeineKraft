uniform float screen_width;
uniform float screen_height;

uniform sampler2D normal_sampler;
uniform sampler2D depth_sampler;
uniform sampler2D position_sampler;
uniform sampler2D diffuse_sampler;

uniform mat4 camera_view;

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

// Enabled/Disable Phong shading
uniform bool lightning_enabled;

#define FLAG_SSAO

#ifdef FLAG_SSAO
uniform sampler2D ssao_sampler;
#endif // FLAG_SSAO

void main() {
    outColor = vec4(1.0); // Sets a default color of white to all objects
    vec4 default_light = vec4(1.0, 1.0, 1.0, 1.0);
    vec2 frag_coord = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);
    vec3 normal = texture(normal_sampler, frag_coord).xyz;
    vec3 position = texture(position_sampler, frag_coord).xyz;

    /// SSAO
    float ambient_occlusion = 0.0;
#ifdef FLAG_SSAO
    ambient_occlusion = texture(ssao_sampler, frag_coord.xy).r;
#endif

#ifdef FLAG_BLINN_PHONG_SHADING
    vec3 total_light = vec3(0.0, 0.0, 0.0);
    vec3 eye = normalize(camera_position - position);
    for (int i = 0; i < MAX_NUM_LIGHTS; i++) {
        Light light = lights[i];
        vec4 light_pos_view_space = camera_view * vec4(light.position, 1.0); // FIXME: Converts to view space ...
        vec3 light_pos = light_pos_view_space.xyz;

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

    if (!lightning_enabled) {
      outColor = vec4(vec3(ambient_occlusion), 1.0);
    }

    outColor = texture(diffuse_sampler, frag_coord).rgba;
}
