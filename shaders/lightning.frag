#version 410 core 

#define M_PI 3.1415926535897932384626433832795

uniform float screen_width;
uniform float screen_height;

uniform sampler2D normal_sampler;
uniform sampler2D depth_sampler;
uniform sampler2D position_sampler;
uniform sampler2D diffuse_sampler;
uniform sampler2D pbr_parameters_sampler;

uniform vec3 camera; // TEST

out vec4 outColor; // Defaults to zero when the frag shader only has 1 out variable

struct PBRInputs {
    vec3 L;
    vec3 V;
    vec3 N;
    vec3 H; // Halfvector 
    float metallic;
    float roughness;
    vec3 base_color;
};

vec3 fresnel_schlick(vec3 F0, vec3 V, vec3 H) {
    return F0 + (vec3(1.0) - F0) * pow(1.0 - dot(V, H), 5.0);
}

float geometric_occlusion_schlick(float roughness, vec3 N, vec3 V, vec3 H) {
    float k = roughness * sqrt(2 / M_PI);
    float GV = dot(V, H) / dot(V, H) * (1.0 - k) + k;
    float GN = dot(N, H) / dot(N, H) * (1.0 - k) + k;
    return GV * GN;
}

float microfaced_distribution_trowbridge_reitz(float a, vec3 N, vec3 H) {
    return a * a / M_PI * pow(pow(dot(N, H), 2) * (a * a - 1.0) + 1.0, 2);
}

vec3 schlick_brdf(PBRInputs inputs) {
    const vec3 dieletric_specular = vec3(0.04);
    const vec3 black = vec3(0.0);
    vec3 cdiff = mix(inputs.base_color * (1 - dieletric_specular.r), black, inputs.metallic);
    vec3 normal_incidence = mix(dieletric_specular, inputs.base_color, inputs.metallic); // F0/R(0) (Fresnel)
    float alpha = inputs.roughness * inputs.roughness;

    vec3 F = fresnel_schlick(normal_incidence, inputs.V, inputs.H);
    vec3 diffuse = inputs.base_color / M_PI; 
    vec3 fdiffuse = (1.0 - F) * diffuse;

    float G = geometric_occlusion_schlick(alpha, inputs.N, inputs.V, inputs.H);
    float D = microfaced_distribution_trowbridge_reitz(inputs.roughness, inputs.N, inputs.H);
    vec3 fspecular = F * G * D / 4 * dot(inputs.N, inputs.L) * dot(inputs.N, inputs.V);

    vec3 f = dot(inputs.V, inputs.L) * (fdiffuse + fspecular);
    
    return f;
}

void main() {
    vec2 frag_coord = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);
    
    vec3 normal = texture(normal_sampler, frag_coord).xyz;
    vec3 position = texture(position_sampler, frag_coord).xyz;
    vec4 diffuse = texture(diffuse_sampler, frag_coord).rgba;
    
    PBRInputs pbr_inputs;

    // TEST 
    pbr_inputs.L = normalize(vec3(0.0, 4.0, 0.0) - position);
    pbr_inputs.V = normalize(camera - position);

    // Metallic roughness material model glTF specific 
    pbr_inputs.metallic = texture(pbr_parameters_sampler, frag_coord).b;
    pbr_inputs.roughness = texture(pbr_parameters_sampler, frag_coord).g;  
    pbr_inputs.N = normal;
    pbr_inputs.H = normalize(pbr_inputs.L + pbr_inputs.V);
    pbr_inputs.base_color = diffuse.rgb;

    vec3 color = schlick_brdf(pbr_inputs);
    
    outColor = vec4(color, 1.0);
}
