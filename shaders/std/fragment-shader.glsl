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

    outColor = texture(diffuse_sampler, frag_coord).rgba;

#ifdef FLAG_BLINN_PHONG_SHADING
    vec3 total_light = vec3(0.0, 0.0, 0.0);
    float specular_power = 32; // FIXME: Uniform
    for (int i = 0; i < MAX_NUM_LIGHTS; i++) {
        Light light = lights[i];
        vec4 light_pos_view_space = camera_view * vec4(light.position, 1.0); // FIXME: Converts to view space ...
        vec3 light_pos = light_pos_view_space.xyz;

        float ambient = 1.0 - ambient_occlusion;

        vec3 direction = normalize(light_pos - position);
        float diffuse = max(dot(normal, direction), 0.0);

        vec3 reflection = reflect(-direction, normal);
        vec3 eye = normalize(-position); // View space eye = (0, 0, 0): A to B = 0 to B = -B
        float specular = pow(max(dot(reflection, eye), 0.0), specular_power);

        total_light = (ambient + diffuse + specular) * light.color.xyz;
    }
   default_light = vec4(total_light, 1.0);
   outColor *= default_light;
#endif

    if (!lightning_enabled) {
       outColor = texture(diffuse_sampler, frag_coord).rgba;
    }
}
