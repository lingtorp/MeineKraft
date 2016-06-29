#version 330 core   // tex.coords (s, t, r) == (x, y, z)

in vec3 fTexcoord;   // passthrough shading for interpolated textures
uniform samplerCube tex;

out vec4 outColor;

void main() {
  outColor = texture(tex, fTexcoord);
}
