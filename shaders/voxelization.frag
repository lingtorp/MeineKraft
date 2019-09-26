
layout(RGBA8) uniform image3D voxel_data;

in vec3 fNormal;   
in vec3 fPosition; // World space position
in vec2 fTextureCoord;
flat in uint dominant_axis_projected;

uniform vec3 aabb_size; 

ivec3 voxel_coordinate_from_world_pos(vec3 pos) {
    vec3 vpos = pos / aabb_size;
    vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
    const uvec3 vgrid = imageSize(voxel_data).xyz; 
    vpos = vgrid * (vpos * vec3(0.5) + vec3(0.5));
    return ivec3(vpos);
}

void main() {
  const ivec3 vpos = voxel_coordinate_from_world_pos(fPosition);
  imageStore(voxel_data, vpos, vec4(1.0));
}
