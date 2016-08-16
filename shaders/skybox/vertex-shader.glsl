#version 330 core

in vec3 position;

// Model
uniform mat4 model;

// View
uniform mat4 camera;

// Projection
uniform mat4 projection;

out vec3 fTexcoord;

void main() {
  gl_Position = projection * camera * model * vec4(position, 1.0f);
  fTexcoord = normalize(position);
}
