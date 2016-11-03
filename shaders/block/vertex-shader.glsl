#version 330 core

in vec3 position;
in vec4 vColor;
in vec3 normal; // Polygon normal
in vec2 vTexCoord;

// Model
in mat4 model;

out vec4 fColor; // This name must match the name in the fragment shader in order to work
out vec3 fTexcoord;
out vec2 tex_coord;

// View or a.k.a camera matrix
uniform mat4 camera_view;

// Projection or a.k.a perspective matrix
uniform mat4 projection;

const vec3 fog_color   = vec3(0.5, 0.5, 0.5);
const vec3 light_color = vec3(0.7, 0.7, 0.7);
const float fog_max_distance = 150;

void main() {
  gl_Position = projection * camera_view * model * vec4(position, 1.0);
  // fTexcoord = normalize(position); // Blocks
  fTexcoord = vec3(vTexCoord, 1.0);
  tex_coord = vTexCoord;

  // Linear fog = 0, Exponential fog = 1, sqrt exponential fog, Disabled = -1
  int fog_type = -1;
  float distance = length(gl_Position);
  if (fog_type == 0) { // linear
    float fog_factor = (fog_max_distance - distance) / (fog_max_distance - (fog_max_distance / 2));
    fog_factor = clamp(fog_factor, 0.0, 1.0);
    fColor = vec4(fog_color * fog_factor, 1.0);
  } else if (fog_type == 1) { // exponential
    float fog_density = 0.0010;
    float fog_factor = 1 / exp(distance * fog_density);
    fog_factor = clamp(fog_factor, 0.0, 1.0);
    fColor = vec4(fog_color * fog_factor, 1.0);
  } else if (fog_type == 2) { // sqrt exponential
    float fog_density = 0.0100;
    float fog_factor = 1 / exp(sqrt(distance * fog_density));
    fog_factor = clamp(fog_factor, 0.0, 1.0);
    fColor = vec4(fog_color * fog_factor, 1.0);
  } else if (fog_type == -1) {
    fColor = vec4(1, 1, 1, 0);
    // fColor = fColor * 1/(length(gl_Position) * 0.04);
  }
}
