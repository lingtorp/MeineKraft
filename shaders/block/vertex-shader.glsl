#version 330 core

in vec3 position;
in vec4 vColor;

out vec4 fColor; // This name must match the name in the fragment shader in order to work

// Model
uniform mat4 transform_x; // the transformation matrix for x
uniform mat4 transform_y; // the transformation matrix for y
uniform mat4 transform_z; // the transformation matrix for z
uniform mat4 transform_translation;
uniform mat4 transform_scaling;

// View
uniform mat4 camera_view;

// Projection
uniform mat4 transform_perspective;

out vec3 fTexcoord;

void main() {
  mat4 model = transform_z * transform_y * transform_x * transform_translation * transform_scaling;
  mat4 projection = transform_perspective;
  gl_Position = projection * camera_view * model * vec4(position, 1.0f);
  fColor = vColor;
  fTexcoord = normalize(position);
}
