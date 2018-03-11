uniform float screen_width;
uniform float screen_height;

uniform sampler2D normal_sampler;
uniform sampler2D depth_sampler;
uniform sampler2D position_sampler;
uniform sampler2D diffuse_sampler;

out vec4 outColor; // Defaults to zero when the frag shader only has 1 out variable

struct Light {
    vec3 color;
    float radius;
    // Attenuation values
    float constant;
    float linear;
    float quadratic;
    // Intensities over RGB
    vec3 ambient_intensity;
    vec3 diffuse_intensity;
    vec3 specular_intensity;
    vec3 position;
};

uniform Light light;

// Enabled/Disable Phong shading
uniform bool lightning_enabled;
uniform float specular_power;
uniform bool blinn_phong;
uniform mat4 camera_view; // For stupid light

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

    outColor = texture(diffuse_sampler, frag_coord).rgba; // FIXME: Use specular map?

#ifdef FLAG_BLINN_PHONG_SHADING
    vec3 total_light = vec3(0.0, 0.0, 0.0);

    vec3 ambient = vec3(1.0 - ambient_occlusion) * light.ambient_intensity;

    vec3 direction = normalize(light.position - position);
    vec3 diffuse = max(dot(normal, direction), 0.0) * light.diffuse_intensity;

    // FIXME: Specular light too much when angles is 90*
    // FIXME: Remove conditionals with clever math functions
    vec3 reflection = reflect(-direction, normal);
    vec3 eye = normalize(-position); // View space eye = (0, 0, 0): A to B = 0 to B = -B
    vec3 specular;
    if (blinn_phong) {
        vec3 half_way = normalize(direction + eye);
        specular = vec3(pow(max(dot(normal, half_way), 0.0), specular_power)) * light.specular_intensity;
    } else {
        specular = vec3(pow(max(dot(reflection, eye), 0.0), specular_power)) * light.specular_intensity;
    }
    // TODO: Toggle individual light contributions
    total_light += ambient;
    total_light += diffuse;
    total_light += specular;
    total_light *= light.color.xyz;

    vec4 light_position = camera_view * vec4(light.position, 1.0);
    float distance = length(light_position.xyz - position);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    // total_light *= attenuation;

    outColor *= vec4(total_light, 1.0);
#endif

    if (!lightning_enabled) {
       outColor = texture(diffuse_sampler, frag_coord).rgba;
    }

    outColor = vec4(1.0);
    // outColor = vec4(vec3(attenuation) * 100, 1.0);
}
