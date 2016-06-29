#version 330 core   // tex.coords (s, t, r) == (x, y, z)

in vec4 fColor; // This name must match the name in the vertex shader in order to work
out vec4 outColor;

in vec3 fTexcoord;   // passthrough shading for interpolated textures
uniform samplerCube tex;

void main() {
  outColor = texture(tex, fTexcoord); // * fColor;
  // outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
  // outColor = fColor;
}
