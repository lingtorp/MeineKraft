#version 330 core   // tex.coords (s, t, r) == (x, y, z)

in  vec4 fColor; // This name must match the name in the vertex shader in order to work
out vec4 outColor;

in vec3 fTexcoord;   // passthrough shading for interpolated textures
in vec2 tex_coord;
// uniform samplerCube tex;
uniform sampler2D tex_sampler;

const vec3 light_pos   = vec3(0, 1, 0);
const vec3 light_color = vec3(1, 1, 1);

void main() {
   outColor = texture(tex_sampler, tex_coord);
}
