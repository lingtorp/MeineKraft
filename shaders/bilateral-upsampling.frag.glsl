// NOTE: Joint bilateral upsampling pass
// File: bilateral-upsampling.frag.glsl

// Low-res src texture
uniform vec2 uInput_pixel_size;   // 1.0 / size
uniform vec2 uInput_texture_size; // size
uniform sampler2D uInput;

// Full-res result upsampled texture
uniform vec2 uOutput_pixel_size;   // 1.0 / size
uniform vec2 uOutput_texture_size; // size
out vec3 uOutput;

// Guide textures (full resolution = high res and downsampled = low res)
uniform bool uPosition_weight;
uniform sampler2D uPosition_high_res; // World space
uniform sampler2D uPosition_low_res;

uniform bool uNormal_weight;
uniform sampler2D uNormal_high_res;
uniform sampler2D uNormal_low_res;

// Normal mapping
uniform bool uNormal_mapping;
uniform sampler2D uTangent_normal;
uniform sampler2D uTangent;

uniform bool uDepth_weight;
uniform sampler2D uDepth_high_res;
uniform sampler2D uDepth_low_res;

/// 'p' is high-res pixel coordinate: [0, high-res-size]
/// returns bottom left texel in low-res normalized texture space: [0, 1]
vec2 get_bottom_left_low_res_texel(const vec2 p) {
  const vec2 t = p * uInput_pixel_size; // [0, 1] in low-res
  return (floor(t * uInput_texture_size - 0.5) + 0.5) / uInput_texture_size;
}

/// Coarse bilinear weights from high-res texture coordinates: BL, BR, TL, TR
/// 'p' is in high-res screen space: [0.5, high-res-size - 0.5]
/// 'BL' is bottom left texel in low-res texture space: [0, 1]
vec4 get_bilinear_weights(const vec2 p, const vec2 BL) {
  const vec2 t = p * uInput_pixel_size; // [0, 1] in low-res
  const vec2 f = fract(t - BL) / (1.0 / uInput_texture_size);
  vec4 weights;
  weights.r = (1.0 - f.x) * (1.0 - f.y); // Bottom left
  weights.g = f.x * (1.0 - f.y);         // Bottom right
  weights.b = (1.0 - f.x) * f.y;         // Top left
  weights.a = f.x * f.y;                 // Top right
  return weights;
}

/// Depths weights depending on similarity to high-res depth
/// Src: https://www.ppsloan.org/publications/ProxyPG.pdf
vec4 get_depth_weights(const vec2 p, const vec2 coarse_samples[4]) {
  const float fine_depth = linearize_depth(texture(uDepth_high_res, p).r);

  vec4 weights;
  for (uint i = 0; i < 4; i++) {
    const float coarse = linearize_depth(texture(uDepth_low_res, coarse_samples[i]).r);
    const float diff = abs(coarse - fine_depth);
    weights[i] = min(1.0 / (diff + EPSILON), 1.0); // FIXME: in [0, 1]?
  }

  return weights;
}

/// Normal weights from the coarse samples
/// Src: https://www.ppsloan.org/publications/ProxyPG.pdf
vec4 get_normal_weights(const vec2 p, const vec2 coarse_samples[4]) {
  const vec3 fNormal = texture(uNormal_high_res, p).xyz;
  vec3 normal = fNormal;

  if (uNormal_mapping) {
    // Tangent normal mapping & TBN tranformation
    const vec3 T = normalize(texture(uTangent, p).xyz);
    const vec3 B = normalize(cross(normal, T));
    const vec3 N = normalize(normal);
    mat3 TBN = mat3(1.0);

    TBN = mat3(T, B, N);
    const vec3 tangent_normal = normalize(2.0 * (texture(uTangent_normal, p).xyz - vec3(0.5)));
    normal = normalize(TBN * tangent_normal);
  }

  vec4 weights;
  for (uint i = 0; i < 4; i++) {
    const vec3 coarse = normalize(texture(uNormal_low_res, coarse_samples[i]).xyz);
    weights[i] = min(max(pow(dot(normal, coarse), 32), 0.0), 1.0);
  }

  return weights;
}

/// Computes weights based on the positional aspect of the guide texture
/// NOTE: Unclear what this actually contributes with to the filtering
vec4 get_position_weights(const vec2 p, const vec2 coarse_samples[4]) {
  const vec3 fine = texture(uPosition_high_res, p).xyz;

  vec4 weights;
  for (uint i = 0; i < 4; i++) {
    const vec3 coarse = texture(uPosition_low_res, coarse_samples[i]).xyz;
    const float dist = max(1.0 / (EPSILON + distance(fine, coarse)), 1.0);
    weights[i] = dist;
  }

  return weights;
}

void main() {
  // Coarse samples: bottom-left, bottom-right, top-left, top-right
  const vec2 BL = get_bottom_left_low_res_texel(gl_FragCoord.xy);
  const vec2 BR = BL + vec2((1.0 / uInput_texture_size).x, 0.0);
  const vec2 TL = BL + vec2(0.0, (1.0 / uInput_texture_size).y);
  const vec2 TR = BL + vec2(1.0 / uInput_texture_size);
  const vec2 coarse_samples[4] = {BL, BR, TL, TR};

  vec4 weights = get_bilinear_weights(gl_FragCoord.xy, BL);

  const vec2 p = gl_FragCoord.xy * uOutput_pixel_size; // [0, 1] in high-res

  if (uDepth_weight) {
    const vec4 depth_weights = get_depth_weights(p, coarse_samples);
    weights *= depth_weights;
  }

  if (uNormal_weight) {
    const vec4 normal_weights = get_normal_weights(p, coarse_samples);
    weights *= normal_weights;
  }

  if (uPosition_weight) {
    const vec4 position_weights = get_position_weights(p, coarse_samples);
    weights *= position_weights;
  }

  // Samples
  float total_weight = 0.0;
  for (uint i = 0; i < 4; i++) {
    uOutput += weights[i] * texture(uInput, coarse_samples[i]).rgb;
    total_weight += weights[i];
  }

  uOutput /= total_weight;
}
