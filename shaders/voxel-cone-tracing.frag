// #version 450                    

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
  const float step_lng = uScaling_factor;
  const float voxel_size = step_lng;
  float opacity = 0.0;
  vec3 radiance = vec3(0.0);

  uint s = 1;
  const uint MAX_STEPS = 10;
  while (s < MAX_STEPS && opacity < 0.99) {

    vec3 world_position = origin + step_lng * direction * s;
    if (!is_inside_scene(world_position)) { break; }
    
    const float cone_diameter = 2 * tan(half_angle) * distance(origin, world_position);
    const float mip = clamp(log2(cone_diameter / voxel_size), 1.0, 4.0);
    const vec3 p = world_to_voxelspace(world_position);
    const float a = textureLod(uVoxelOpacity, p, mip).r;

    // Front-to-back acculumation
    radiance += (1.0 - opacity) * a * textureLod(uVoxelRadiance, p, mip).rgb;
    opacity += (1.0 - opacity) * a;

    s++;
  }

  return vec4(radiance, opacity);
}

void main() {
  const vec2 frag_coord = vec2(gl_FragCoord.x / uScreen_width, gl_FragCoord.y / uScreen_height);

  // Diffuse cones
  const vec3 origin = texture(uPosition, frag_coord).xyz;

  vec3 normal = texture(uNormal, frag_coord).xyz;

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
  const float roughness = 1.0; // = texture(uPBR_parameters, frag_coord).g; // pow(x, 2) // convert to perceptual roighnesss
  const float metallic = 1.0; // = texture(uPBR_parameters, frag_coord).b;

  // Diffuse cones
  color += trace_cone(origin, TBN * normal, 45.0 * roughness);
  #define NUM_DIFFUSE_CONES 4
  for (uint i = 0; i < NUM_DIFFUSE_CONES; i++) {
    const vec3 direction = TBN * normalize(uCone_directions[i]);
    color += trace_cone(origin, direction, 45.0 * roughness) * dot(direction, normal);
  }

  // Specular cone
  const vec3 reflection = reflect(-normalize(uCamera_position - origin), normal);
  color += trace_cone(origin, TBN * reflection, 10.0 * metallic); // * dot(reflection, normal);

  const vec3 diffuse = texture(uDiffuse, frag_coord).rgb;
  color.rgb *= diffuse;

  // color.rgb = texture(uVoxelRadiance, world_to_voxelspace(origin)).rgb;
}
