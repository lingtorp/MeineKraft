
layout(RGBA8) uniform image3D voxel_data;

in vec3 fNormal;   
in vec3 fPosition; // World space position
in vec2 fTextureCoord;

layout(RGBA8) uniform writeonly image3D uVoxels;
uniform sampler2DArray uDiffuse;

uniform vec3 aabb_size; 

ivec3 voxel_coordinate_from_world_pos(vec3 pos) {
  vec3 vpos = pos / aabb_size;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  const vec3 vgrid = vec3(imageSize(uVoxels).xyz);
  vpos = vgrid * (vpos * vec3(0.5) + vec3(0.5));
  return ivec3(vpos);
}

void main() {
  const ivec3 vpos = voxel_coordinate_from_world_pos(fPosition);
  const vec3 diffuse = texture(uDiffuse, vec3(fTextureCoord, 0)).rgb;
  imageStore(uVoxels, vpos, vec4(diffuse, 1.0));
  // imageStore(uVoxels, vpos, vec4(1.0));
}
