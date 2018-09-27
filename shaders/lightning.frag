#version 430 core 

#define M_PI 3.1415926535897932384626433832795

uniform float screen_width;
uniform float screen_height;

uniform sampler2D normal_sampler;
uniform sampler2D depth_sampler;
uniform sampler2D position_sampler;
uniform sampler2D diffuse_sampler;
uniform sampler2D pbr_parameters_sampler;
uniform sampler2D ambient_occlusion_sampler;
uniform sampler2D emissive_sampler;

uniform vec3 camera; // TEST

out vec4 outColor; // Defaults to zero when the frag shader only has 1 out variable

struct PBRInputs {
    vec3 L;
    vec3 V;
    vec3 N;
    vec3 H; // Halfvector 
    float NdotL;
    float NdotV;
    float VdotL;
    float VdotH;
    float NdotH;
    float LdotH;
    float metallic;
    float roughness;
    vec3 base_color;
};

vec3 fresnel_schlick(vec3 F0, PBRInputs inputs) {
    return F0 + (vec3(1.0) - F0) * pow(1.0 - inputs.VdotH, 5.0);
}

float geometric_occlusion_schlick(PBRInputs inputs) {
    float k = inputs.roughness * sqrt(2.0 / M_PI);
    float GL = inputs.LdotH / (inputs.LdotH * (1.0 - k) + k);
    float GN = inputs.NdotH / (inputs.NdotH * (1.0 - k) + k);
    return GL * GN;
}

float microfaced_distribution_trowbridge_reitz(PBRInputs inputs) {
    float a = inputs.roughness;
    return a * a / (M_PI * pow(pow(inputs.NdotH, 2) * (a * a - 1.0) + 1.0, 2.0));
}

vec3 diffuse_lambertian(PBRInputs inputs) {
    return inputs.base_color / M_PI;
}

vec3 schlick_brdf(PBRInputs inputs) {
    const vec3 dieletric_specular = vec3(0.04);
    const vec3 black = vec3(0.0);
    vec3 cdiff = mix(inputs.base_color * (1.0 - dieletric_specular.r), black, inputs.metallic);
    vec3 normal_incidence = mix(dieletric_specular, inputs.base_color, inputs.metallic); // F0/R(0) (Fresnel)
    inputs.roughness *= inputs.roughness; // Convert to material roughness from perceptual roughness

    vec3 F = fresnel_schlick(normal_incidence, inputs);
    vec3 diffuse = diffuse_lambertian(inputs); 
    vec3 fdiffuse = (1.0 - F) * diffuse;

    float G = geometric_occlusion_schlick(inputs);
    float D = microfaced_distribution_trowbridge_reitz(inputs);
    vec3 fspecular = (F * G * D) / (4.0 * inputs.NdotL * inputs.NdotV);

    vec3 f = fdiffuse + fspecular;
    
    return f;
}

vec3 SRGB_to_linear(vec3 srgb) {
    // TODO: Look this up
    // vec3 bLess = step(vec3(0.04045), srgb.xyz);
    // return mix(srgb.xyz/vec3(12.92), pow((srgb.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess);
    return pow(srgb, vec3(2.2)); // Fast approximation
}

void main() {
    const vec2 frag_coord = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);
    
    const vec3 normal = texture(normal_sampler, frag_coord).xyz;
    const vec3 position = texture(position_sampler, frag_coord).xyz;
    const vec3 diffuse = SRGB_to_linear(texture(diffuse_sampler, frag_coord).rgb); // Mandated by glTF 2.0
    const vec3 ambient_occlusion = texture(ambient_occlusion_sampler, frag_coord).rgb;
    const vec3 emissive = texture(emissive_sampler, frag_coord).rgb;

    PBRInputs pbr_inputs;

    // TEST 
    vec3 light_intensities = vec3(23.47, 21.31, 20.79);
    vec3 light_position = vec3(0.0, 3.0, 0.0);

    pbr_inputs.L = normalize(light_position - position);
    float distance = length(light_position - position);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = attenuation * light_intensities;

    // Metallic roughness material model glTF specific 
    pbr_inputs.V = normalize(camera - position);
    pbr_inputs.metallic = texture(pbr_parameters_sampler, frag_coord).b;
    pbr_inputs.roughness = texture(pbr_parameters_sampler, frag_coord).g;  
    pbr_inputs.N = normalize(normal);
    pbr_inputs.H = normalize(pbr_inputs.L + pbr_inputs.V);
    pbr_inputs.base_color = diffuse.rgb;
    pbr_inputs.NdotL = clamp(dot(pbr_inputs.N, pbr_inputs.L), 0.001, 1.0);
    pbr_inputs.NdotV = clamp(abs(dot(pbr_inputs.N, pbr_inputs.V)), 0.001, 1.0);
    pbr_inputs.VdotL = clamp(dot(pbr_inputs.V, pbr_inputs.L), 0.0, 1.0);
    pbr_inputs.VdotH = clamp(dot(pbr_inputs.V, pbr_inputs.H), 0.0, 1.0);
    pbr_inputs.NdotH = clamp(dot(pbr_inputs.N, pbr_inputs.H), 0.0, 1.0);
    pbr_inputs.LdotH = clamp(dot(pbr_inputs.L, pbr_inputs.H), 0.0, 1.0);

    vec3 ambient = vec3(0.3) * diffuse * ambient_occlusion; 
    vec3 color = ambient + radiance * schlick_brdf(pbr_inputs) * pbr_inputs.NdotL;

    // Emissive
    color += emissive; // TODO: Emissive factor missing

    // Tone mapping (using Reinhard operator)
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
