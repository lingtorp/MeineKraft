// #version 450

const float M_PI = 3.141592653589793;

uniform float uScreen_height;
uniform float uScreen_width;

uniform sampler3D uVoxelRadiance;
uniform sampler3D uVoxelOpacity;

uniform bool normalmapping;
uniform sampler2D uTangent;
uniform sampler2D uTangent_normal;

uniform vec3[4] uCone_directions;

uniform vec3 uCamera_position;

uniform sampler2D uDiffuse;
uniform sampler2D uPosition; // World space
uniform sampler2D uNormal;
uniform sampler2D uPBR_parameters;

uniform float uScaling_factor;
uniform uint uVoxel_grid_dimension;
uniform vec3 uAABB_center;
uniform vec3 uAABB_min;
uniform vec3 uAABB_max;

// User customizable
uniform int   uMax_steps;
uniform float uRoughness;
uniform float uMetallic;
uniform float uRoughness_aperature; // Radians
uniform float uMetallic_aperature;  // Radians
uniform bool  uDirect_lighting;
uniform bool  uIndirect_lighting; 

ivec3 voxel_coordinate_from_world_pos(const vec3 pos) {
  vec3 vpos = (pos - uAABB_center) * uScaling_factor;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  const vec3 vgrid = vec3(uVoxel_grid_dimension);
  vpos = vgrid * (vpos * vec3(0.5) + vec3(0.5));
  return ivec3(vpos);
}

vec3 world_to_voxelspace(const vec3 pos) {
  vec3 vpos = (pos - uAABB_center) * uScaling_factor;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  return vpos * vec3(0.5) + vec3(0.5);
}

bool is_inside_scene(const vec3 p) {
  if (p.x > uAABB_max.x || p.x < uAABB_min.x) { return false; }
  if (p.y > uAABB_max.y || p.y < uAABB_min.y) { return false; }
  if (p.z > uAABB_max.z || p.z < uAABB_min.z) { return false; }
  return true;
}

out vec4 color;

vec4 trace_cone(const vec3 origin,
                const vec3 direction,
                const float half_angle) {
  const float voxel_size_LOD0 = uScaling_factor;

  float opacity = 0.0;
  vec3 radiance = vec3(0.0);
  float cone_distance = voxel_size_LOD0; // Avoid self-occlusion

  uint s = 0;
  while (s < uMax_steps && opacity < 0.99) {

    const vec3 world_position = origin + cone_distance * direction;
    if (!is_inside_scene(world_position)) { break; }
    
    const float cone_diameter = max(2.0 * tan(half_angle) * cone_distance, voxel_size_LOD0);
    cone_distance += cone_diameter;

    const float mip = clamp(log2(cone_diameter / voxel_size_LOD0), 1.0, 4.0);

    const vec3 p = world_to_voxelspace(world_position);
    const float a = textureLod(uVoxelOpacity, p, mip).r;

    // Front-to-back acculumation
    radiance += (1.0 - opacity) * a * textureLod(uVoxelRadiance, p, mip).rgb;
    opacity += (1.0 - opacity) * a;

    s++;
  }

  return vec4(radiance, opacity);
}

uniform float uShadow_bias;
uniform vec3 uDirectional_light_direction;
uniform mat4 uLight_space_transform;
uniform sampler2D uShadowmap;

bool shadow(const vec3 world_position, const vec3 normal) {
  vec4 lightspace_position = uLight_space_transform * vec4(world_position, 1.0);
  lightspace_position.xyz /= lightspace_position.w;
  lightspace_position = lightspace_position * 0.5 + 0.5;

  const float current_depth = lightspace_position.z;

  if (current_depth < 1.0) { 
    const float closest_shadowmap_depth = texture(uShadowmap, lightspace_position.xy).r;
    
    // Bias avoids the _majority_ of shadow acne
    const float bias = uShadow_bias * dot(-uDirectional_light_direction, normal);

    return closest_shadowmap_depth < current_depth - bias;
  }
  return false;
}
  
void main() {
  const vec2 frag_coord = vec2(gl_FragCoord.x / uScreen_width, gl_FragCoord.y / uScreen_height);

  // Diffuse cones
  const vec3 origin = texture(uPosition, frag_coord).xyz;

  vec3 normal = texture(uNormal, frag_coord).xyz;
  vec3 fNormal = normal; // NOTE: Using geometric normal due to minimal jitter in hard shadows produced

  // Tangent normal mapping & TBN tranformation
  const vec3 T = normalize(texture(uTangent, frag_coord).xyz);
  const vec3 B = normalize(cross(normal, T));
  const vec3 N = normal;
  mat3 TBN = mat3(1.0);

  if (normalmapping) {
    TBN = mat3(T, B, N);
    const vec3 tangent_normal = normalize(2.0 * (texture(uTangent_normal, frag_coord).xyz - vec3(0.5)));
    normal = normalize(TBN * tangent_normal);  
  }

  // NOTE: These produce insane amounts of noise 
  const float roughness = texture(uPBR_parameters, frag_coord).g; // pow(x, 2) // convert to perceptual roighnesss
  const float metallic = texture(uPBR_parameters, frag_coord).b;
  const float roughness_aperature = uRoughness_aperature;
  const float metallic_aperature = uMetallic_aperature;

  // Diffuse cones
  color += trace_cone(origin, TBN * normal, roughness_aperature * roughness);
  #define NUM_DIFFUSE_CONES 4
  for (uint i = 0; i < NUM_DIFFUSE_CONES; i++) {
    const vec3 direction = TBN * normalize(uCone_directions[i]);
    color += trace_cone(origin, direction, roughness_aperature * roughness) * dot(direction, normal);
  }

  // Specular cone
  const vec3 reflection = reflect(normalize(uCamera_position - origin), normal);
  color += trace_cone(origin, reflection, metallic_aperature * metallic); 

  const vec3 diffuse = texture(uDiffuse, frag_coord).rgb;
  color.rgb *= diffuse;

  if (!uIndirect_lighting) {
    color.rgb = vec3(0.0);
  }

  if (uDirect_lighting) {
    if (!shadow(origin, fNormal)) {
      color.rgb += diffuse * max(dot(-uDirectional_light_direction, normal), 0.0);
    }
  }

  // Gamma correction
  // color.rgb = pow(color.rgb, vec3(1.0 / 2.2));

  // color.rgb = texture(uVoxelRadiance, world_to_voxelspace(origin)).rgb;

  // color.rgb = texture(uDiffuse, frag_coord).rgb;
}
