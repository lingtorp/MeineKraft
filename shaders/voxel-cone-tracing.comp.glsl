// NOTE: Performs voxel cone tracing for every Nth pixel

// #version 450 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint uNth_pixel;

layout(RGBA8) uniform restrict volatile writeonly image2D uScreen;

uniform uint uScreen_height; // Pixels
uniform uint uScreen_width;  // Pixels

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
layout(std140, binding = 8) readonly buffer DiffuseCones {
  vec4 cones[];
};

// Clipmap level passed on log2 assumption between levels
float clipmap_lvl_from_distance(const vec3 position) {
  const float AABB_LOD0_radius = 0.5 * (1.0 / uScaling_factors[0]);
  const float d = distance(position, uAABB_centers[0]) / AABB_LOD0_radius;
  // return 0.0; FIXME:
  // d in [0, 2)
  if (d < 2.0) {
    return d;
  }
  // d in [2, inf] from 2 hence log2(x) + 1 >= 2 where x >= 2
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

  const vec4 s1 = sample_clipmap(wp, lvl1);

  const vec4 s0s1 = mix(s0, s1, fract(lvl));
  return vec4(s0s1.rgb * (lvl - fract(lvl)) * 0.1, s0s1.a); // NOTE: Deals with the rings
}

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

  while (cone_distance < max_distance && opacity < 1.0) {
    const vec3 cone_position = origin + cone_distance * direction;

    const float cone_diameter = max(2.0 * tan(half_angle) * cone_distance, uVoxel_size_LOD0);
    cone_distance += cone_diameter * 1.0; // 0.5 for cone tracing with radius step size

    const float min_lvl  = floor(clipmap_lvl_from_distance(cone_position));
    const float curr_lvl = log2(cone_diameter / uVoxel_size_LOD0);
    const float lvl = min(max(max(start_lvl, curr_lvl), min_lvl), float(NUM_CLIPMAPS - 1));

    // Front-to-back acculumation without pre-multiplied alpha
    const vec4 sampled = sample_clipmap_linearly(cone_position, lvl);
    radiance  += (1.0 - opacity) * sampled.a * sampled.rgb;
    opacity   += (1.0 - opacity) * sampled.a;

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

  while (cone_distance < max_distance && occlusion < 1.0) {
    const vec3 cone_position = origin + cone_distance * direction;

    const float cone_diameter = max(2.0 * tan(half_angle) * cone_distance, uVoxel_size_LOD0);
    cone_distance += cone_diameter;

    const float min_lvl = floor(clipmap_lvl_from_distance(cone_position));
    const float curr_lvl = log2(cone_diameter / uVoxel_size_LOD0);
    const float lvl = min(max(max(start_lvl, curr_lvl), min_lvl), float(NUM_CLIPMAPS - 1));

    // Front-to-back acculumation without pre-multiplied alpha
    const vec4 sampled = sample_clipmap_linearly(cone_position, lvl);
    radiance  += (1.0 - occlusion) * sampled.a * sampled.rgb;
    occlusion += (1.0 - occlusion) * sampled.a;
  }

  return vec4(radiance, 1.0 - occlusion);
}

uniform uint uShadow_algorithm;
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

// FIXME: Blocky shadows due to clipmap level sampling not 100%
float vct_shadow(const vec3 origin, const vec3 normal, const vec3 direction) {
  const float half_angle = 0.0050; // Shadow cone aperature

  const float max_distance = (1.0 / uScaling_factors[NUM_CLIPMAPS - 1]) / 8.0;
  const float start_lvl = floor(clipmap_lvl_from_distance(origin));

  float occlusion = 0.0;
  float cone_distance = 0.0;
  const vec3 o = origin + (uVoxel_size_LOD0 * 1.5 * exp2(start_lvl)) * normal; // Avoids self-occlusion/accumulation

  while (cone_distance < max_distance) {
    const vec3 cone_position = o + cone_distance * direction;

    // FIXME: Restrict shadow cones to clipmap AABBs to inc. perf.
    // if (!is_inside_AABB(uAABB_mins[NUM_CLIPMAPS - 1], uAABB_maxs[NUM_CLIPMAPS - 1], cone_position)) {
    //  return occlusion;
    // }

    const float cone_diameter = max(2.0 * tan(half_angle) * cone_distance, uVoxel_size_LOD0);
    cone_distance += cone_diameter * 1.0;

    const float min_lvl = floor(clipmap_lvl_from_distance(cone_position));
    const float curr_lvl = log2(cone_diameter / uVoxel_size_LOD0);
    const float lvl = min(max(max(start_lvl, curr_lvl), min_lvl), float(NUM_CLIPMAPS - 1));

    // Front-to-back acculumation
    occlusion += (1.0 - occlusion) * sample_clipmap_linearly(cone_position, lvl).a;
  }

  return 1.0 - smoothstep(0.1, 0.95, occlusion);
}

void main() {
  vec4 color = vec4(0.0); // (RGB, Opacity/Occlusion)

  const vec2 screen_dims = vec2(uScreen_width, uScreen_height);
  const vec2 frag_coord = (uNth_pixel * gl_GlobalInvocationID.xy) / screen_dims;

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

  uint traced_cones = 1;
  if (uDiffuse_lighting) {
    // Offset origin to avoid self-sampling
    const float start_lvl = floor(clipmap_lvl_from_distance(origin));
    const vec3 o = origin + (uVoxel_size_LOD0 * 1.5 * exp2(start_lvl)) * fNormal;
    const vec4 radiance = cones[0].w * diffuse * trace_diffuse_cone(o, normal, roughness_aperature);
    color += radiance;

    for (uint i = 1; i < uNum_diffuse_cones; i++) {
      // Offset origin to avoid self-sampling
      const vec3 d = cones[i].xyz;
      if (dot(d, normal) < 0.0) { continue; }
      const vec4 radiance = 2.0 * cones[i].w * diffuse * trace_diffuse_cone(o, d, roughness_aperature);
      color.rgb += radiance.rgb * max(dot(d, normal), 0.0);
      color.a   += radiance.a;
      traced_cones++;
    }
  }

  if (uAmbient_lighting) {
    color.rgb += diffuse.rgb * vec3(color.a * M_PI_INV) / traced_cones;

    // color.rgb = vec3(0.0);
    // color.rgb += vec3(color.a) / traced_cones;
    // NOTE: See generate_diffuse_cones for details about the division
  }

  if (uSpecular_lighting) {
    const float aperture = metallic_aperature;
    const vec3 reflection = normalize(reflect(-(uCamera_position - origin), fNormal));
    color += metallic * trace_specular_cone(origin, reflection, aperture);
  }

  if (uDirect_lighting) { // TODO: fNormal vs. normal?
    float shadow = 0.0;
    switch (uShadow_algorithm) {
      case 0:
        shadow = plain_shadow(origin, normal);
        break;
      case 1:
        shadow = pcf_shadow(origin, normal);
        break;
      case 2:
        shadow = vct_shadow(origin, normal, -uDirectional_light_direction);
        break;
    }
    color.rgb += shadow * diffuse.rgb * max(dot(-uDirectional_light_direction, normal), 0.0);
    // color.rgb += vec3(shadow);
  }

  color.rgb += texture(uEmissive, frag_coord).rgb;

  // color.rgb = vec3(floor(clipmap_lvl_from_distance(origin)) / NUM_CLIPMAPS);
  // color.rgb = sample_clipmap(origin, uint(floor(clipmap_lvl_from_distance(origin)))).rgb;

  // if (is_inside_AABB(uAABB_mins[0], uAABB_maxs[0], origin)) {
  //   color.rgb = sample_clipmap(origin, 0).rgb;
  //	} else {
  //   color.rgb = vec3(0.0);
  // }
  // color.rgb = sample_clipmap(origin, 0).rgb; // St?mmer inte ?verrens med distance funktionen!
  // color.rgb = vec3(floor(clipmap_lvl_from_distance(origin)) == 0.0 ? 1.0 : 0.0);

  // color.rgb = vec3(world_to_clipmap_voxelspace(origin, uScaling_factors[0], uAABB_centers[0]));
  // color.rgb = sample_clipmap_linearly(origin, clipmap_lvl_from_distance(origin)).rgb;
  // color.rgb = vec3(floor(clipmap_lvl_from_distance(origin)) / NUM_CLIPMAPS);

  imageStore(uScreen, ivec2(frag_coord * screen_dims), color);
}
