uniform mat4 projection;
uniform mat4 camera_view;

in mat4 model;
in vec3 position;
in vec3 normal;

out vec3 fNormal;
out vec3 fPosition;

void main() {
    gl_Position = projection * camera_view * model * vec4(position, 1.0);
    fNormal = normal;
    fPosition = vec3(camera_view * model * vec4(position, 1.0));
}