
in vec3 position;

flat out uint vertex_ids;

void main() {
  gl_Position = vec4(position, 1.0);
  vertex_ids = gl_VertexID;
}
