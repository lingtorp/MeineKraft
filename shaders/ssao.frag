#version 410 core 

uniform mat4 projection;

uniform sampler2D normal_sampler;
uniform sampler2D noise_sampler;
uniform sampler2D position_sampler;

uniform int num_ssao_samples;
uniform vec3 ssao_samples[512];
uniform float ssao_kernel_radius;
uniform float ssao_power;
uniform float ssao_bias;

const vec2 noise_scale = vec2(1280.0 / 8.0, 720.0 / 8.0);

out float ambient_occlusion;

void main() {
    vec2 frag_coord = vec2(gl_FragCoord.x / 1280.0, gl_FragCoord.y / 720.0);
    vec3 normal = texture(normal_sampler, frag_coord.xy).xyz;
    vec3 position = texture(position_sampler, frag_coord.xy).xyz;

    // Orientate kernel sample hemisphere
    vec3 rvec = texture(noise_sampler, gl_FragCoord.xy * noise_scale).xyz;
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal); // f: Tangent -> View space

    ambient_occlusion = 0.0;
    for (int i = 0; i < num_ssao_samples; i++) {
        // 1. Get sample
        vec3 sampled = position + tbn * ssao_samples[i] * ssao_kernel_radius;

        // 2. Generate sample depth
        vec4 point = vec4(sampled, 1.0);
        point = projection * point;
        point.xy /= point.w;
        point.xy = point.xy * 0.5 + 0.5;

        // 3. Lookup depth at sample's position
        float point_depth = texture(position_sampler, point.xy).z;

        // 4. Compare
        if (point_depth >= sampled.z + ssao_bias) { ambient_occlusion += 1.0; }
    }
    ambient_occlusion = 1.0 - (ambient_occlusion / float(num_ssao_samples));
    ambient_occlusion = pow(ambient_occlusion, ssao_power);
}