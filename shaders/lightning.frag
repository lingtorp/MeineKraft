#version 410 core 

uniform float screen_width;
uniform float screen_height;

uniform sampler2D normal_sampler;
uniform sampler2D depth_sampler;
uniform sampler2D position_sampler;
uniform sampler2D diffuse_sampler;
uniform sampler2D pbr_parameters_sampler;

out vec4 outColor; // Defaults to zero when the frag shader only has 1 out variable

void main() {
    vec2 frag_coord = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);
    
    vec3 normal = texture(normal_sampler, frag_coord).xyz;
    vec3 position = texture(position_sampler, frag_coord).xyz;
    vec4 diffuse = texture(diffuse_sampler, frag_coord).rgba;
    
    // Metallic roughness material model
    vec2 metallic_roughness = texture(pbr_parameters_sampler, frag_coord).bg; // glTF specific 
    
    outColor = vec4(vec3(0.0, metallic_roughness.gr), 1.0);
}
