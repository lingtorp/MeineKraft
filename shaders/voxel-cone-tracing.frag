// NOTE: Performs fullscreen VCT

uniform uint uScreen_height;
uniform uint uScreen_width;

uniform sampler3D uVoxel_radiance[NUM_CLIPMAPS];
uniform sampler3D uVoxel_opacity[NUM_CLIPMAPS];

uniform bool uNormalmapping;
uniform sampler2D uTangent;
uniform sampler2D uTangent_normal;

uniform vec3 uCamera_position;
uniform vec3 uDirectional_light_intensity;
uniform vec3 uDirectional_light_direction;
uniform float uVCT_shadow_cone_aperature; // Shadow cone aperature (deg.)

// General textures
uniform sampler2D uPosition; // World space
uniform sampler2D uNormal;
uniform sampler2D uPBR_parameters;

// Out textures
layout(location = 0) out vec3  gIndirect_radiance;
layout(location = 1) out float gAmbient_radiance;
layout(location = 2) out vec3  gSpecular_radiance;
layout(location = 3) out vec3  gDirect_radiance;

// Voxel cone tracing related
uniform float uVoxel_size_LOD0;
uniform float uScaling_factors[NUM_CLIPMAPS];
uniform vec3  uAABB_centers[NUM_CLIPMAPS];
uniform vec3  uAABB_mins[NUM_CLIPMAPS];
uniform vec3  uAABB_maxs[NUM_CLIPMAPS];
uniform float uAmbient_decay;                  // Crassin11 mentions but does not specify

// User customizable
uniform float uRoughness_aperature; // Radians (half-angle of cone)
uniform float uMetallic_aperature;  // Radians (half-angle of cone)

// Computational toggles
uniform bool uIndirect;
uniform bool uSpecular;
uniform bool uAmbient;
uniform bool uDirect;

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
  const vec3 radiance = texture(uVoxel_radiance[lvl], p).rgb;
  const float opacity = texture(uVoxel_opacity[lvl], p).r;
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

    occlusion += (1.0 - occlusion) * sampled.a / (1.0 + cone_distance * uAmbient_decay);
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

// FIXME: Blocky shadows due to clipmap level sampling not 100%
float vct_shadow(const vec3 origin, const vec3 normal, const vec3 direction) {
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

    const float cone_diameter = max(2.0 * tan(uVCT_shadow_cone_aperature) * cone_distance, uVoxel_size_LOD0);
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
  const vec2 frag_coord = vec2(gl_FragCoord.x / float(uScreen_width), gl_FragCoord.y / float(uScreen_height));
  
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

  // Indirect
  {
    float ambient_radiance = 0.0; // NOTE: Traced with diffuse cones
    uint traced_cones = 1;

    // Offset origin to avoid self-sampling
    const float start_lvl = floor(clipmap_lvl_from_distance(origin));
    const vec3 o = origin + (uVoxel_size_LOD0 * 1.5 * exp2(start_lvl)) * fNormal; 

    const vec4 radiance = 2.0 * cones[0].w * trace_diffuse_cone(o, normal, roughness_aperature);
    gIndirect_radiance += radiance.rgb;
    ambient_radiance += radiance.a;

    for (uint i = 1; i < uNum_diffuse_cones; i++) {
      // Offset origin to avoid self-sampling
      const vec3 d = cones[i].xyz;
      if (dot(d, normal) < 0.0) { continue; }
      const vec3 d = TBN * cones[i].xyz;
      const vec4 radiance = 2.0 * cones[i].w * trace_diffuse_cone(o, d, roughness_aperature);
      gIndirect_radiance += radiance.rgb * max(dot(d, normal), 0.0);
      ambient_radiance += radiance.a;
      traced_cones++;
    }

    if (!uIndirect) {
      gIndirect_radiance = vec3(0.0);
    }

    if (uAmbient) {
      // NOTE: See generate_diffuse_cones for details about the division of M_PI
      gAmbient_radiance = (ambient_radiance * M_PI_INV) / float(traced_cones);
    }
  }

  if (uSpecular) {
    const float aperture = metallic_aperature; 
    const vec3 reflection = normalize(reflect(-(uCamera_position - origin), fNormal));
    gSpecular_radiance = trace_specular_cone(origin, reflection, aperture).rgb;
  }

  if (uDirect) {
    const float shadow = vct_shadow(origin, normal, -uDirectional_light_direction);
    gDirect_radiance = shadow * uDirectional_light_intensity * max(dot(-uDirectional_light_direction, normal), 0.0);
  }

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
}
