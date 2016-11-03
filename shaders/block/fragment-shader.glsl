#version 330 core   // tex.coords (s, t, r) == (x, y, z)

in  vec4 fColor; // This name must match the name in the vertex shader in order to work
out vec4 outColor;

in vec3 fTexcoord;   // passthrough shading for interpolated textures
in vec2 tex_coord;
// uniform samplerCube tex;
uniform sampler1D tex_sampler;
// uniform sampler2d tex_sampler;

void main() {
  // outColor = texture(tex, fTexcoord) * fColor;
  outColor = texture(tex_sampler, tex_coord.s);
}
