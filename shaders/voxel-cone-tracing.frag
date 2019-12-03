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
uniform bool  uDiffuse_lighting;
uniform bool  uSpecular_lighting;
uniform bool  uAmbient_lighting;

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
float clipmap_lvl_from_distance(const vec3 position) {
  const float AABB_LOD0_radius = 0.5 * (1.0 / uScaling_factors[0]);
  const float d = distance(position, uAABB_centers[0]) / AABB_LOD0_radius;
  // d in [0, 2)
  if (d < 2.0) {
    return d;
  }
  // d in [2, inf] from 2 hence log2(2) + 1 = 2
  return log2(d) + 1;
}

// Beware of sampling outside the clipmap!
vec4 sample_clipmap(const vec3 wp, const uint lvl) {
  const vec3 p = world_to_clipmap_voxelspace(wp, uScaling_factors[lvl], uAABB_centers[lvl]);
  const vec3 radiance = texture(uVoxelRadiance[lvl], p).rgb;
  const float opacity = texture(uVoxelOpacity[lvl], p).r;
  return vec4(radiance, opacity);
}

// Sample point in world space
vec4 sample_clipmap_linearly(const vec3 wp, const float lvl) {
  const uint lvl0 = uint(floor(lvl));

  const vec4 s0 = sample_clipmap(wp, lvl0);

  const uint lvl1 = uint(ceil(lvl));

  if (lvl0 == lvl1) { return s0; }
  if (lvl1 == NUM_CLIPMAPS) { return s0; }

  // NOTE: Cool little hack mixes in Ambient lighting when clipmaps are exhausted.
  // This is just another way to provide ambient lighting for the scene
  // const vec4 AMBIENT = vec4(0.1); // Scene dependent of course
  // if (lvl1 == NUM_CLIPMAPS) { return mix(s0, AMBIENT, fract(lvl)); }

  const vec4 s1 = sample_clipmap(wp, lvl1);

  return mix(s0, s1, fract(lvl));
}

out vec4 color;

uniform float uVoxel_size_LOD0;

vec4 trace_diffuse_cone(const vec3 origin,
                        const vec3 direction,
                        const float half_angle) {
  const float max_distance = (1.0 / uScaling_factors[NUM_CLIPMAPS - 1]) / 1.0; 
  const float start_lvl = floor(clipmap_lvl_from_distance(origin));

  float occlusion = 0.0;
  float opacity = 0.0;
  vec3 radiance = vec3(0.0);
  float cone_distance = 0.0;
  float ambient_occlusion = 0.0;

  while (cone_distance < max_distance && opacity < 1.0) {
    const vec3 cone_position = origin + cone_distance * direction;

    const float cone_diameter = max(2.0 * tan(half_angle) * cone_distance, uVoxel_size_LOD0);
    cone_distance += cone_diameter * 1.0; // 0.5 for cone tracing with radius step size
    
    const float min_lvl = floor(clipmap_lvl_from_distance(cone_position));
    const float curr_lvl = log2(cone_diameter / uVoxel_size_LOD0);
    const float lvl = min(max(max(start_lvl, curr_lvl), min_lvl), float(NUM_CLIPMAPS));

    // Front-to-back acculumation without pre-multiplied alpha
    const vec4 sampled = sample_clipmap_linearly(cone_position, lvl);
    radiance  += (1.0 - opacity) * sampled.a * sampled.rgb;
    opacity   += (1.0 - opacity) * sampled.a;

    const float voxel_size = uVoxel_size_LOD0 * exp2(floor(lvl));
    const float decay = 0.1; // Crassin11 mentions but does not specify
    occlusion += (1.0 - occlusion) * sampled.a / (1.0 + cone_distance * decay);
  }

  return vec4(radiance, 1.0 - occlusion);
}

// FIXME: Specular cones still self-accumulate a fair bit
vec4 trace_specular_cone(const vec3 origin,
                         const vec3 direction,
                         const float half_angle) {
  const float max_distance = (1.0 / uScaling_factors[NUM_CLIPMAPS - 1]) / 4.0; 
  const float start_lvl = floor(clipmap_lvl_from_distance(origin));

  float occlusion = 0.0;
  vec3 radiance = vec3(0.0);
  float cone_distance = uVoxel_size_LOD0 * exp2(start_lvl); // Avoids self-occlusion/accumulation
  float ambient_occlusion = 0.0;

  while (cone_distance < max_distance && occlusion < 1.0) {
    const vec3 cone_position = origin + cone_distance * direction;

    const float cone_diameter = max(2.0 * tan(half_angle) * cone_distance, uVoxel_size_LOD0);
    cone_distance += cone_diameter;
    
    const float min_lvl = floor(clipmap_lvl_from_distance(cone_position));
    const float curr_lvl = log2(cone_diameter / uVoxel_size_LOD0);
    const float lvl = min(max(max(start_lvl, curr_lvl), min_lvl), float(NUM_CLIPMAPS));

    // Front-to-back acculumation without pre-multiplied alpha
    const vec4 sampled = sample_clipmap_linearly(cone_position, lvl);
    radiance  += (1.0 - occlusion) * sampled.a * sampled.rgb;
    occlusion += (1.0 - occlusion) * sampled.a;
  }

  return vec4(radiance, 1.0 - occlusion);
}

uniform float uShadow_bias;
uniform vec3 uDirectional_light_direction;
uniform mat4 uLight_space_transform;
uniform sampler2D uShadowmap;
uniform uint uShadowmap_width;
uniform uint uShadowmap_height;

float plain_shadow(const vec3 world_position, const vec3 normal) {
  vec4 lightspace_position = uLight_space_transform * vec4(world_position, 1.0);
  lightspace_position.xyz /= lightspace_position.w;
  lightspace_position = lightspace_position * 0.5 + 0.5;

  const float current_depth = lightspace_position.z;

  const float closest_shadowmap_depth = texture(uShadowmap, lightspace_position.xy).r;
    
  // Bias avoids the _majority_ of shadow acne
  const float bias = uShadow_bias * dot(-uDirectional_light_direction, normal);

  return (closest_shadowmap_depth < current_depth - bias) ? 0.0 : 1.0;
}

uniform uint uPCF_shadow_samples; // TODO: Hook up

// Percentage-closer filtering shadow technique
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

float vct_shadow(const vec3 world_position, const vec3 normal) {
  // TODO: Implement VCT for shadows 
  return 0.0;
}
  
void main() {
  const vec2 frag_coord = vec2(gl_FragCoord.x / uScreen_width, gl_FragCoord.y / uScreen_height);

  const vec3 origin = texture(uPosition, frag_coord).xyz;
  const vec3 fNormal = texture(uNormal, frag_coord).xyz; 
  vec3 normal = fNormal;

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
  const vec4  diffuse = sRGB_to_linear(vec4(texture(uDiffuse, frag_coord).rgb, 1.0));

  if (uDiffuse_lighting) {
    color += cones[0].w * diffuse * trace_diffuse_cone(origin, normal, roughness_aperature);
    for (uint i = 1; i < uNum_diffuse_cones; i++) {
	  // Offset origin to avoid self-occlusion/accumulation
	  const float start_lvl = floor(clipmap_lvl_from_distance(origin));
	  const vec3 o = origin + (uVoxel_size_LOD0 * 1.5 * exp2(start_lvl)) * fNormal; 
      const vec3 direction = normalize(TBN * cones[i].xyz);
      color += 2.0 * cones[i].w * diffuse * trace_diffuse_cone(o, direction, roughness_aperature); // * max(dot(direction, normal), 0.0);
    }
  }

  if (uAmbient_lighting) {
    color.rgb = vec3(0.0);
    color.rgb += vec3(color.a / 2.0) / uNum_diffuse_cones;
    // NOTE: See generate_diffuse_cones for details about the division
  }

  if (uSpecular_lighting) {
    const float aperture = metallic_aperature; 
    const vec3 reflection = normalize(reflect(-(uCamera_position - origin), fNormal));
    color += metallic * trace_specular_cone(origin, reflection, aperture);
  }

  if (!uIndirect_lighting) {
    color.rgb = vec3(0.0);
  }

  if (uDirect_lighting) { // TODO: fNormal vs. normal?
    float shadow = 0.0;
    switch (1) {
      case 0:
        shadow = plain_shadow(origin, normal);
        break;
      case 1:
        shadow = pcf_shadow(origin, normal);
        break;
      case 2:
        shadow = vct_shadow(origin, normal);
        break;
    }
    color.rgb += shadow * diffuse.rgb * max(dot(-uDirectional_light_direction, normal), 0.0);
  }

  color.rgb += texture(uEmissive, frag_coord).rgb;

  // TODO: Tonemap?
  // color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
  // color.rgb = linear_to_sRGB(color.rgb);

  // color.rgb = vec3(roughness);
  // color.rgb = vec3(metallic);

  // color.rgb = vec3(floor(clipmap_lvl_from_distance(origin)) / NUM_CLIPMAPS);
  // color.rgb = sample_clipmap(origin, uint(floor(clipmap_lvl_from_distance(origin)))).rgb;
  // color.rgb = sample_clipmap_linearly(origin, clipmap_lvl_from_distance(origin)).rgb;
  // color.rgb = vec3(floor(clipmap_lvl_from_distance(origin)) == 3.0 ? 1.0 : 0.0);
  // color.rgb = vec3(floor(clipmap_lvl_from_distance(origin)) / NUM_CLIPMAPS);
}
