
in vec3 fNormal;   
in vec3 fPosition; // World space position
in vec2 fTextureCoord;

uniform uint voxel_grid_dimension;
layout(RGBA8) uniform writeonly image3D uVoxelRadiance; // restrict
// FIXME: Does not work for RG8 ...
layout(RGBA8) uniform writeonly image3D uVoxelOpacity; // restrict

uniform sampler2DArray uDiffuse;
uniform sampler2D uShadowmap;
uniform mat4 light_space_transform;

uniform vec3 aabb_size;
uniform vec3 light_direction; // Directional light direction

ivec3 voxel_coordinate_from_world_pos(vec3 pos) {
  vec3 vpos = pos / aabb_size;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  const vec3 vgrid = vec3(voxel_grid_dimension);
  vpos = vgrid * (vpos * vec3(0.5) + vec3(0.5));
  return ivec3(vpos);
}

void main() {
  const ivec3 vpos = voxel_coordinate_from_world_pos(fPosition);

  vec4 lightspace_position = light_space_transform * vec4(fPosition, 1.0);
  lightspace_position.xyz /= lightspace_position.w;
  lightspace_position = lightspace_position * 0.5 + 0.5;
  const float voxel_depth = lightspace_position.z;

  if (voxel_depth < 1.0) { 
    const float closest_shadowmap_depth = texture(uShadowmap, lightspace_position.xy).r;
    
    // Bias avoids _some_ shadowmap acne
    const float shadow_bias = 0.005; // max(0.005 * (1.0 - clamp(dot(normal, directional_light_direction), 0.0, 1.0)), 0.0005);

    // Inject radiance if voxel NOT in shadow
    const bool shadow = closest_shadowmap_depth < voxel_depth - shadow_bias ? true : false;
    if (!shadow) {
      const vec3 diffuse = texture(uDiffuse, vec3(fTextureCoord, 0)).rgb;
      const vec3 radiance = diffuse * max(dot(light_direction, fNormal), 0.0);
      imageStore(uVoxelRadiance, vpos, vec4(radiance, 1.0));
    }
  }

  imageStore(uVoxelOpacity, vpos, vec4(1.0)); 
}
