// NOTE: Computes direct lighting stores in Gbuffer direct radiance texture
// File: direct-lighting.frag.glsl

uniform uint uScreen_height;
uniform uint uScreen_width;

uniform bool uNormalmapping;
uniform sampler2D uTangent;
uniform sampler2D uTangent_normal;

uniform vec3 uCamera_position;

uniform uint uShadow_algorithm;
uniform float uShadow_bias;
uniform vec3 uDirectional_light_intensity;
uniform vec3 uDirectional_light_direction;
uniform mat4 uLight_space_transform;
uniform sampler2D uShadowmap;
uniform uint uShadowmap_width;
uniform uint uShadowmap_height;
uniform uint uPCF_samples;

// General textures
uniform sampler2D uPosition;
uniform sampler2D uNormal;

out vec3 gDirect_radiance;

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

// Percentage-closer filtering shadow technique
float pcf_shadow(const vec3 world_position, const vec3 normal) {
  vec4 s = uLight_space_transform * vec4(world_position, 1.0);
  s.xyz /= s.w;
  s = s * 0.5 + 0.5;

  const float current_depth = s.z;
  const float num_samples = uPCF_samples; // 2 ==> 5x5 kernel, n ==> (n + 1)x(n + 1) kernel

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
  const vec2 frag_coord = gl_FragCoord.xy / vec2(float(uScreen_width), float(uScreen_height));

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

  // TODO: fNormal vs. normal?
  float shadow = 0.0;
  switch (uShadow_algorithm) {
    case 0:
    shadow = plain_shadow(origin, normal);
    break;
    case 1:
    shadow = pcf_shadow(origin, normal);
    break;
  }
  gDirect_radiance = shadow * uDirectional_light_intensity * max(dot(-uDirectional_light_direction, normal), 0.0);
}
