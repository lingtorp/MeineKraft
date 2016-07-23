#version 330 core

in vec3 position;

// Model
uniform mat4 model;

// View
uniform mat4 camera_view;

// Projection
uniform mat4 projection;

out vec3 fTexcoord;

void main() {
  gl_Position = projection * camera_view * model * vec4(position, 1.0f);
  fTexcoord = normalize(position);
}
