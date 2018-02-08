in vec3 position;
in mat4 model;

uniform mat4 camera_view;
uniform mat4 projection;

void main() {
  gl_Position = projection * camera_view * model * vec4(position, 1.0);
}
