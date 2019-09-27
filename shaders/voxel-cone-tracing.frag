
uniform float uScreen_height;
uniform float uScreen_width;

uniform sampler3D uVoxels;

uniform sampler2D uDiffuse;
uniform sampler2D uPosition; // World space
uniform vec3 uCamera_position;

uniform vec3 aabb_size; 

ivec3 voxel_coordinate_from_world_pos(vec3 pos) {
  vec3 vpos = pos / aabb_size;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  const vec3 vgrid = vec3(16.0);
  vpos = vgrid * (vpos * vec3(0.5) + vec3(0.5));
  return ivec3(vpos);
}

out vec4 color;

void main() {
  const vec2 frag_coord = vec2(gl_FragCoord.x / uScreen_width, gl_FragCoord.y / uScreen_height);

  /// Painting diffuse
  const vec3 diffuse = texture(uDiffuse, frag_coord).rgb;
  color = vec4(diffuse, 1.0);

  /// Volume rendering 
  // const vec3 origin = uCamera_position;
  // const vec3 direction = normalize(texture(uPosition, frag_coord).xyz - uCamera_position);
  // const float STEP_SIZE = 0.05;
  // float opacity = 0.0;
  // for (int s = 0; s < 10; s++) {
  //   const vec3 p = origin + direction * s;
  //   color += texture(uVoxels, p);
  // }

  /// Surface visualization
  // const vec3 position = texture(uPosition, frag_coord).rgb;
  // const vec3 voxel_value = texture(uVoxels, position).rgb;
  // color = vec4(voxel_value, 1.0);
}
