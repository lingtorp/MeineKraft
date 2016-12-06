#version 330 core   // tex.coords (s, t, r) == (x, y, z)

in  vec4 fColor; // This name must match the name in the vertex shader in order to work
out vec4 outColor;

in vec3 fTexcoord;   // passthrough shading for interpolated textures
in vec2 tex_coord;
// uniform samplerCube tex;
uniform sampler2D diffuse_sampler;

in vec3 fNormal;
in vec4 fPosition;

/// Lights
struct Light {
    vec4 color;
    vec3 position;
};

const int MAX_NUM_LIGHTS = 1;

layout (std140) uniform lights_block {
//    uint number_lights;
    Light lights[MAX_NUM_LIGHTS];
};

void main() {
    // materialAmbient, materialDiffuse, materialSpecular
    vec3 total_light = vec3(0.0, 0.0, 0.0);
    float ambient = 0.3;
    total_light += ambient;

    for (int i = 0; i < MAX_NUM_LIGHTS; i++) {
        Light light = lights[i];
        vec3 distance = normalize(lights[i].position.xyz - fPosition.xyz);
        vec3 diffuse_light = light.color.xyz * max(dot(fNormal, distance), 0.0) / distance*distance; // diffuse
        total_light += diffuse_light;

        vec3 reflected = reflect(vec3(-1.0, -1.0, -1.0), fNormal); // specular
        float cosAlpha = clamp(dot(distance, reflected), 0, 1);
        vec3 specular = light.color.xyz * 4 * pow(cosAlpha, 7.0) / distance*distance;
        // light += specular; // Not working quite right
    }

   total_light = clamp(total_light, 0.0, 1.0);
   outColor = texture(diffuse_sampler, tex_coord) * vec4(total_light, 1.0);
}
