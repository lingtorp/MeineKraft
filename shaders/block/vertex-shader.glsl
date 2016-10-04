#version 330 core

in vec3 position;
in vec4 vColor;

// Model
in mat4 model;

out vec4 fColor; // This name must match the name in the fragment shader in order to work
out vec3 fTexcoord;

// View or a.k.a camera matrix
uniform mat4 camera_view;

// Projection or a.k.a perspective matrix
uniform mat4 projection;

const vec3 fog_color = vec3(0.5, 0.5, 0.5);
const float fog_max_distance = 200;

void main() {
  gl_Position = projection * camera_view * model * vec4(position, 1.0f);
  fTexcoord = normalize(position);

  // Linear fog
  if (false) {
    float distance = length(gl_Position);
    float fog_factor = (fog_max_distance - distance)/(fog_max_distance - 100);
    fog_factor = clamp(fog_factor, 0.0, 1.0);
    fColor = vec4(fog_color * fog_factor, 1.0);
  }
  fColor = vec4(1, 1, 1, 0);
}
