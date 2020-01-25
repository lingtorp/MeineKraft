
#define NUM_CLIPMAPS 4

layout(points) in;
layout(triangle_strip, max_vertices = 14) out;

flat in uint vertex_ids[];

layout(RGBA8) uniform readonly image3D uVoxel_radiance[NUM_CLIPMAPS];
layout(RGBA8) uniform readonly image3D uVoxel_opacity[NUM_CLIPMAPS];

uniform mat4 uCamera_view;
uniform uint uClipmap_idx;
uniform int  uClipmap_sizes[NUM_CLIPMAPS];
uniform vec3 uAABB_mins[NUM_CLIPMAPS];
uniform float uScaling_factors[NUM_CLIPMAPS];

#define NUM_VERTICES_CUBE 14
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

out vec4 voxel_color;

void main() {
  const uint vertex_id = vertex_ids[0]; // Only one per point anyways

  const uint voxel_grid_dimension = uClipmap_sizes[uClipmap_idx];
  const uint X = vertex_id % voxel_grid_dimension;
  const uint Y = (vertex_id / voxel_grid_dimension) % voxel_grid_dimension;
  const uint Z = (vertex_id / (voxel_grid_dimension * voxel_grid_dimension)) % voxel_grid_dimension;
  const ivec3 vpos = ivec3(X, Y, Z);

  const vec4 opacity = imageLoad(uVoxel_opacity[uClipmap_idx], vpos);
  if (opacity == vec4(1.0)) {
    voxel_color = vec4(imageLoad(uVoxel_radiance[uClipmap_idx], vpos).rgb, 1.0);

    const float voxel_size = (1.0f / float(uScaling_factors[uClipmap_idx])) / float(voxel_grid_dimension);
    const vec3 voxel_center = vec3(vpos * voxel_size) + uAABB_mins[uClipmap_idx] + vec3(voxel_size / 2.0);

    for (uint i = 0; i < NUM_VERTICES_CUBE; i++) {
      const vec3 vertex = voxel_center + voxel_size * cube_strip[i];
      gl_Position = uCamera_view * vec4(vertex, 1.0);
      EmitVertex();
    }

    EndPrimitive();
  }
}
