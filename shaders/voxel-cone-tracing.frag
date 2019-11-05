// #version 450 core

#define NUM_CLIPMAPS 4

const float M_PI = 3.141592653589793;

uniform float uScreen_height;
uniform float uScreen_width;

uniform sampler3D uVoxelRadiance[NUM_CLIPMAPS];
uniform sampler3D uVoxelOpacity[NUM_CLIPMAPS];

uniform bool uNormalmapping;
uniform sampler2D uTangent;
uniform sampler2D uTangent_normal;

uniform vec3 uCamera_position;

// General textures
uniform sampler2D uDiffuse;
uniform sampler2D uPosition; // World space
uniform sampler2D uNormal;
uniform sampler2D uPBR_parameters;
uniform sampler2D uEmissive;

uniform float uScaling_factors[NUM_CLIPMAPS];
uniform int   uClipmap_sizes[NUM_CLIPMAPS];
uniform vec3  uAABB_centers[NUM_CLIPMAPS];
uniform vec3  uAABB_mins[NUM_CLIPMAPS];
uniform vec3  uAABB_maxs[NUM_CLIPMAPS];

// User customizable
uniform float uRoughness;
uniform float uMetallic;
uniform float uRoughness_aperature; // Radians (half-angle of cone)
uniform float uMetallic_aperature;  // Radians (half-angle of cone)
uniform bool  uDirect_lighting;
uniform bool  uIndirect_lighting;

uniform uint uNum_diffuse_cones;
// (Vec3, float) = (direction, weight) for each cone
layout(std140, binding = 8) buffer DiffuseCones {
  vec4 cones[];
};

// Material handling related stuff
const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

// Linear --> sRGB approximation
// See http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
// Src: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag
vec3 linear_to_sRGB(const vec3 color) {
  return pow(color, vec3(INV_GAMMA));
}

// sRGB to linear approximation
// See http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
// Src: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag
vec4 sRGB_to_linear(const vec4 sRGB) {
  return vec4(pow(sRGB.xyz, vec3(GAMMA)), sRGB.w);
}

bool is_inside_AABB(const vec3 aabb_min, const vec3 aabb_max, const vec3 p) {
  if (p.x > aabb_max.x || p.x < aabb_min.x) { return false; }
  if (p.y > aabb_max.y || p.y < aabb_min.y) { return false; }
  if (p.z > aabb_max.z || p.z < aabb_min.z) { return false; }
  return true;
}

vec3 world_to_clipmap_voxelspace(const vec3 pos,
                                 const float scaling_factor,
                                 const vec3 aabb_center) {
  vec3 vpos = (pos - aabb_center) * scaling_factor;
  vpos = clamp(vpos, vec3(-1.0), vec3(1.0));
  return vpos * vec3(0.5) + vec3(0.5);
}

// Clipmap level passed on log2 assumption between levels
uint clipmap_lvl_from_distance(const vec3 position) {
  const vec3 clipmap_origin = uAABB_centers[0];
  const float AABB_LOD0_radius = (1.0f / uScaling_factors[0]) / 2.0f;
  return uint(log2((distance(position, clipmap_origin) / AABB_LOD0_radius) + 1));
// Sample point in world space
vec4 sample_clipmap_linearly(const vec3 p, const float level) {
  const uint lower_lvl = uint(floor(level));

  const vec3 p0 = world_to_clipmap_voxelspace(p, uScaling_factors[lower_lvl], uAABB_centers[lower_lvl]);

  const vec3  lower_sample_radiance = texture(uVoxelRadiance[lower_lvl], p0).rgb;
  const float lower_sample_opacity  = texture(uVoxelOpacity[lower_lvl], p0).r;

  const uint upper_lvl = uint(ceil(level));

  if (lower_lvl == upper_lvl) {
    return vec4(lower_sample_radiance, lower_sample_opacity);
  }

  const vec3 p1 = world_to_clipmap_voxelspace(p, uScaling_factors[upper_lvl], uAABB_centers[upper_lvl]);

  const vec3  upper_sample_radiance = texture(uVoxelRadiance[upper_lvl], p1).rgb;
  const float upper_sample_opacity  = texture(uVoxelOpacity[upper_lvl], p1).r;

  const vec3  radiance = mix(lower_sample_radiance, upper_sample_radiance, fract(level));
  const float opacity  = mix(lower_sample_opacity, upper_sample_opacity, fract(level));
  return vec4(radiance, opacity);
}

out vec4 color;

uniform float uVoxel_size_LOD0;

vec4 trace_cone(const vec3 origin,
                const vec3 direction,
                const float half_angle) {
  float occlusion = 0.0;
  vec3 radiance = vec3(0.0);

  float cone_distance = uVoxel_size_LOD0; // Avoids self-occlusion/accumlation

  const uint start_clipmap = clipmap_lvl_from_distance(origin);

  while (cone_distance < 100.0 && occlusion < 1.0) {
    const vec3 world_position = origin + cone_distance * direction;

    const float cone_diameter = max(2.0 * tan(half_angle) * cone_distance, uVoxel_size_LOD0);

    // uint clipmap = start_clipmap + uint(log2(cone_distance / uVoxel_size_LOD0));
    const uint curr_clipmap = uint(floor(log2(cone_diameter / uVoxel_size_LOD0)));
    const uint clipmap = min(max(start_clipmap, curr_clipmap), NUM_CLIPMAPS - 1);

    if (!is_inside_AABB(uAABB_mins[clipmap], uAABB_maxs[clipmap], world_position)) { break; }
    
    const vec3 p = world_to_clipmap_voxelspace(world_position, uScaling_factors[clipmap], uAABB_centers[clipmap]);

    // Front-to-back acculumation with pre-multiplied alpha
    const vec4 sampled = sample_clipmap_linearly(cone_position, lvl);
    radiance += (1.0 - occlusion) * sampled.a * sampled.rgb;
    occlusion += (1.0 - occlusion) * sampled.a;

    cone_distance += cone_diameter * 0.5; // Smoother result than whole cone diameter
  }

  return vec4(radiance, occlusion);
}

uniform float uShadow_bias;
uniform vec3 uDirectional_light_direction;
uniform mat4 uLight_space_transform;
uniform sampler2D uShadowmap;
uniform uint uShadowmap_width;
uniform uint uShadowmap_height;

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

// Percentage-close filtering shadow technique
float pcf_shadow(const vec3 world_position, const vec3 normal) {
  vec4 s = uLight_space_transform * vec4(world_position, 1.0);
  s.xyz /= s.w;
  s = s * 0.5 + 0.5;

  const float current_depth = s.z;

  const float num_samples = 2; // 2 ==> 4x4 kernel, n ==> nxn kernel
  float shadowing = 0.0;
  for (float x = -num_samples + 0.5; x < num_samples - 0.5; x += 1.0) {
    for (float y = -num_samples + 0.5; y < num_samples - 0.5; y += 1.0) {
      const vec2 p = vec2(s.x + x * (1.0 / float(uShadowmap_width)),
                          s.y + y * (1.0 / float(uShadowmap_height)));
      const float depth = texture(uShadowmap, p).r;
      shadowing += depth < current_depth ? 0.0 : 1.0;
    }
  }
  return shadowing / (4.0 * num_samples * num_samples);
}
  
void main() {
  const vec2 frag_coord = vec2(gl_FragCoord.x / uScreen_width, gl_FragCoord.y / uScreen_height);

  const vec3 origin = texture(uPosition, frag_coord).xyz;
  vec3 normal = texture(uNormal, frag_coord).xyz;

  // Tangent normal mapping & TBN tranformation
  const vec3 T = normalize(texture(uTangent, frag_coord).xyz);
  const vec3 B = normalize(cross(normal, T));
  const vec3 N = normalize(normal);
  mat3 TBN = mat3(1.0);

  if (uNormalmapping) {
    TBN = mat3(T, B, N);
    const vec3 tangent_normal = normalize(2.0 * (texture(uTangent_normal, frag_coord).xyz - vec3(0.5)));
    normal = normalize(TBN * tangent_normal);  
  }

  // Material parameters
  const float roughness = texture(uPBR_parameters, frag_coord).g; 
  const float metallic = texture(uPBR_parameters, frag_coord).b;
  const float roughness_aperature = uRoughness_aperature;
  const float metallic_aperature = uMetallic_aperature;
  const vec4  diffuse = sRGB_to_linear(texture(uDiffuse, frag_coord));

  // Diffuse cones
  // FIXME: Weights should be the Lambertian cosine factor? 
  color += diffuse * trace_cone(origin, normal, roughness_aperature);
  for (uint i = 1; i < uNum_diffuse_cones; i++) {
    const vec3 direction = normalize(TBN * cones[i].xyz);
    color += diffuse * trace_cone(origin, direction, roughness_aperature) * max(dot(direction, normal), 0.0);
  }
  // color /= uNum_diffuse_cones + 1; // ??
  // color *= (1.0f / M_PI); // ??

  // Specular cone
  const vec3 reflection = normalize(reflect(-(uCamera_position - origin), normal));
  color += trace_cone(origin, reflection, metallic_aperature);

  if (!uIndirect_lighting) {
    color.rgb = vec3(0.0);
  }

  if (uDirect_lighting) {
    if (!shadow(origin, normal)) {
      color.rgb += diffuse.rgb * max(dot(-uDirectional_light_direction, normal), 0.0);
    }
  }

  // Percentage-closer filtering shadows
  // if (uDirect_lighting) {
  //   const float shadowing = pcf_shadow(origin, normal);
  //   color.rgb += shadowing * diffuse.rgb * max(dot(-uDirectional_light_direction, normal), 0.0);
  // }

  color.rgb += texture(uEmissive, frag_coord).rgb;
}
