#version 410 core 

uniform mat4 projection;
uniform mat4 camera_view;

in vec3 position;

out vec3 fPosition;

void main() {
  gl_Position = projection * camera_view * vec4(position, 1.0);
  fPosition = position;
}
