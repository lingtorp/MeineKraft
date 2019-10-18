
#extensions: 

#include "shaders/voxelization-commons.glsl"

layout(local_size_x = VOXEL_GRID_DIMENSION​,
       local_size_y = VOXEL_GRID_DIMENSION​,
       local_size_z = VOXEL_GRID_DIMENSION) in;

// NOTE: (radiance, atomic_counter) --> (radiance, opacity)
layout(R32UI) uniform coherent volatile image3D voxels;

void main() {
  const vec3 vpos = gl_LocalInvocationID.xyz;
  vec4 voxel_value = convert_r32_to_rgba8(imageLoad(voxels, vpos));
  if (voxel_value.a > 0.0) {
    voxel_value = 1.0;
  }
  imageStore(voxels, vpos, convert_rgba8_to_r32(voxel_value));
}
