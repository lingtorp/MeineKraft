uniform float screen_width;
uniform float screen_height;

uniform sampler2D normal_sampler;
uniform sampler2D depth_sampler;
uniform sampler2D position_sampler;
uniform sampler2D diffuse_sampler;

out vec4 outColor; // Defaults to zero when the frag shader only has 1 out variable

/// Lights
struct Light {
    vec3 color;
    vec3 light_intensity; // (ambient, diffuse, specular, padding) - itensities
    vec3 position;
};

uniform Light light;

// Enabled/Disable Phong shading
uniform bool lightning_enabled;
uniform float specular_power;
uniform bool blinn_phong;

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

    float ambient = 1.0 - ambient_occlusion;

    vec3 direction = normalize(light.position - position);
    float diffuse = max(dot(normal, direction), 0.0);

    // FIXME: Specular light too much when angles is 90*
    // FIXME: Remove conditionals with clever math functions
    vec3 reflection = reflect(-direction, normal);
    vec3 eye = normalize(-position); // View space eye = (0, 0, 0): A to B = 0 to B = -B
    float specular;
    if (blinn_phong) {
        vec3 half_way = normalize(direction + eye);
        specular = pow(max(dot(normal, half_way), 0.0), specular_power);
    } else {
        specular = pow(max(dot(reflection, eye), 0.0), specular_power);
    }
    // TODO: Toggle individual light contributions
    total_light = (ambient + diffuse + specular) * light.color.xyz;

   default_light = vec4(total_light, 1.0);
   outColor *= default_light;
#endif

    if (!lightning_enabled) {
       outColor = texture(diffuse_sampler, frag_coord).rgba;
    }
}
