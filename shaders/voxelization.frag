
in vec3 fNormal;   
in vec3 fPosition; // World space position
in vec2 fTextureCoord;

uniform uint voxel_grid_dimension;
layout(RGBA8) uniform writeonly image3D uVoxelRadiance; 
layout(RGBA8) uniform writeonly image3D uVoxelOpacity; 

uniform sampler2DArray uDiffuse;
uniform sampler2D uShadowmap;
uniform mat4 light_space_transform;

uniform vec3 aabb_center;
uniform float scaling_factor;

uniform vec3 light_direction; // Directional light direction

ivec3 voxel_coordinate_from_world_pos(const vec3 pos) {
  vec3 vpos = (pos - aabb_center) * scaling_factor;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  const vec3 vgrid = vec3(voxel_grid_dimension);
  vpos = vgrid * (vpos * vec3(0.5) + vec3(0.5));
  return ivec3(vpos);
}

vec4 conv_RGBA8_to_vec4(uint v) {
  return vec4(float(v & 0x000000FF), float((v & 0x0000FF00) >> 8U),
              float((v & 0x00FF0000) >> 16U), float((v & 0xFF000000) >> 24U));
}

uint conv_vec4_to_uint(vec4 v) {
  return uint((uint(v.w) & 0x000000FF) << 24U | (uint(v.z) & 0x000000FF) << 16U |
              (uint(v.y) & 0x000000FF) << 8U | uint(v.x) & 0x000000FF);
}

void atomic_moving_avg_radiance_to_voxel(layout(R32UI) coherent volatile uimage3D voxels,
                                  const vec4 radiance,
                                  const ivec3 vpos) {
  uint new_value = conv_vec4_to_uint(radiance);
  uint curr_value = 0;
  uint prev_value = 0;
  // Compute moving average until value settles
  while ((curr_value = imageAtomicCompSwap(voxels, vpos, prev_value, new_value)) != prev_value) {
    prev_value = curr_value;
    vec4 value = conv_RGBA8_to_vec4(curr_value);
    value.xyz *= value.w;
    vec4 valuef = value + new_value;
    valuef.xyz /= valuef.w;
    new_value = conv_vec4_to_uint(valuef);
  }
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
    const bool shadow = closest_shadowmap_depth < voxel_depth - shadow_bias;
    if (!shadow) {
      const vec3 diffuse = texture(uDiffuse, vec3(fTextureCoord, 0)).rgb;
      const vec4 radiance = vec4(diffuse * max(dot(fNormal, vec3(0.0)), 1.0), 1.0);
      imageStore(uVoxelRadiance, vpos, radiance);
      // atomic_moving_avg_radiance_to_voxel(uVoxelRadiance, radiance, vpos);
    }
  }

  imageStore(uVoxelOpacity, vpos, vec4(1.0)); 
}
