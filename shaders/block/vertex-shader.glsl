#version 330 core

in vec3 position;
in vec4 vColor;
in vec3 normal; // Polygon normal
in vec2 vTexCoord;

// Model
in mat4 model;

out vec4 fColor; // This name must match the name in the fragment shader in order to work
out vec2 fTexcoord;

// View or a.k.a camera matrix
uniform mat4 camera_view;

// Projection or a.k.a perspective matrix
uniform mat4 projection;

const vec3 fog_color   = vec3(0.5, 0.5, 0.5);
const vec3 light_color = vec3(0.7, 0.7, 0.7);
const float fog_max_distance = 150;

out vec3 fNormal;
out vec4 fPosition;

void main() {
  gl_Position = projection * camera_view * model * vec4(position, 1.0);

  fTexcoord = vTexCoord;
  fNormal   = normal;
  fPosition = model * vec4(position, 1.0);
}
