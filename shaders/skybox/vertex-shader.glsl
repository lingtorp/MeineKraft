#version 330 core

in vec3 position;

// Model
uniform mat4 transform_scaling;
uniform mat4 transform_translation;

// View
uniform mat4 camera_view;

// Projection
uniform mat4 transform_perspective;

out vec3 fTexcoord;

void main() {
  gl_Position = transform_perspective * camera_view * transform_scaling * vec4(position, 1.0f);
  fTexcoord = normalize(position);
}
