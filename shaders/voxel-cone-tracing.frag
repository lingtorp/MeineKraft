
uniform float uScreen_height;
uniform float uScreen_width;

uniform sampler3D uVoxelRadiance;
uniform sampler3D uVoxelOpacity;

uniform sampler2D uDiffuse;
uniform sampler2D uPosition; // World space
uniform sampler2D uNormal;

uniform float uScaling_factor;
uniform uint uVoxel_grid_dimension;
uniform vec3 uAABB_center; 

ivec3 voxel_coordinate_from_world_pos(const vec3 pos) {
  vec3 vpos = (pos - uAABB_center) * uScaling_factor;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  const vec3 vgrid = vec3(uVoxel_grid_dimension);
  vpos = vgrid * (vpos * vec3(0.5) + vec3(0.5));
  return ivec3(vpos);
}

vec3 world_to_voxelspace(vec3 pos) {
  vec3 vpos = (pos - uAABB_center) * uScaling_factor;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  return vpos * vec3(0.5) + vec3(0.5);
}

out vec4 color;

void main() {
  const vec2 frag_coord = vec2(gl_FragCoord.x / uScreen_width, gl_FragCoord.y / uScreen_height);
  const vec3 origin = texture(uPosition, frag_coord).xyz;
  const vec3 direction = texture(uNormal, frag_coord).xyz;
  const float step_lng = uScaling_factor;
  float opacity = 0.0;
  for (int s = 1; s < 10; s++) {
    const vec3 p = world_to_voxelspace(origin + step_lng * direction * s);
    // opacity += textureLod(uVoxelOpacity, p, float(s));
    color += (1.0 - opacity) * textureLod(uVoxelRadiance, p, float(s));
  }
}
