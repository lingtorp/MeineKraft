#version 330 core   // tex.coords (s, t, r) == (x, y, z)

in  vec4 fColor; // This name must match the name in the vertex shader in order to work
out vec4 outColor;

in vec3 fTexcoord;   // passthrough shading for interpolated textures
in vec2 tex_coord;
// uniform samplerCube tex;
uniform sampler2D tex_sampler;

in vec3 fNormal;
in vec4 fPosition;

uniform vec3 light_pos;
const vec3 light_color = vec3(1, 1, 1);
const vec3 light_posa  = vec3(15, 15, 0);

void main() {
    // materialAmbient, materialDiffuse, materialSpecular
   float ambient = 0.3;
   vec3 distance = normalize(light_pos - fPosition.xyz);

   vec3 light = light_color * max(dot(fNormal, distance), 0.0) / distance*distance; // diffuse
   light = light + ambient; // ambient

   vec3 reflected = reflect(vec3(-1.0, -1.0, -1.0), fNormal); // specular
   float cosAlpha = clamp(dot(distance, reflected), 0, 1);
   vec3 specular = light_color*4 * pow(cosAlpha, 7.0) / distance*distance;
   // light = light + specular; // Not working quite right

   light = clamp(light, 0.0, 1.0);
   outColor = texture(tex_sampler, tex_coord) * vec4(light, 1.0);
}
