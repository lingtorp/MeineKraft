
layout(points) in;
layout(triangle_strip, max_vertices = 14) out;

flat in uint vertex_ids[];

layout(RGBA8) uniform readonly image3D uVoxelRadiance;
layout(RGBA8) uniform readonly image3D uVoxelOpacity;

uniform mat4 camera_view;

uniform uint voxel_grid_dimension;
uniform float aabb_max_dimension;
uniform vec3 aabb_min;

out vec4 voxel_color;

const vec3 cube_strip[14] = vec3[14](
  vec3(-1.0, 1.0, 1.0),     // Front-top-left
  vec3(1.0, 1.0, 1.0),      // Front-top-right
  vec3(-1.0, -1.0, 1.0),    // Front-bottom-left
  vec3(1.0, -1.0, 1.0),     // Front-bottom-right
  vec3(1.0, -1.0, -1.0),    // Back-bottom-right
  vec3(1.0, 1.0, 1.0),      // Front-top-right
  vec3(1.0, 1.0, -1.0),     // Back-top-right
  vec3(-1.0, 1.0, 1.0),     // Front-top-left
  vec3(-1.0, 1.0, -1.0),    // Back-top-left
  vec3(-1.0, -1.0, 1.0),    // Front-bottom-left
  vec3(-1.0, -1.0, -1.0),   // Back-bottom-left
  vec3(1.0, -1.0, -1.0),    // Back-bottom-right
  vec3(-1.0, 1.0, -1.0),    // Back-top-left
  vec3(1.0, 1.0, -1.0)      // Back-top-right
);

void main() {
  const uint vertex_id = vertex_ids[0];
  const uint X = vertex_id % voxel_grid_dimension;
  const uint Y = (vertex_id / voxel_grid_dimension) % voxel_grid_dimension;
  const uint Z = (vertex_id / (voxel_grid_dimension * voxel_grid_dimension)) % voxel_grid_dimension;
  const ivec3 vpos = ivec3(X, Y, Z);

  const vec4 opacity = imageLoad(uVoxelOpacity, vpos);
  if (opacity == vec4(1.0)) {
    voxel_color = vec4(imageLoad(uVoxelRadiance, vpos).rgb, 1.0);
    voxel_color = vec4(1.0);
    const float voxel_size = float(aabb_max_dimension) / float(voxel_grid_dimension);
    const vec3 voxel_center = vec3(vpos * voxel_size) + aabb_min + vec3(voxel_size / 2.0);
    for (uint i = 0; i < 14; i++) {
      const vec3 vertex = voxel_center + voxel_size * cube_strip[i];
      gl_Position = camera_view * vec4(vertex, 1.0);
      EmitVertex();
    }
    EndPrimitive();
  }
}
