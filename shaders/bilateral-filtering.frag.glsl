// NOTE: Joint bilateral filtering pass
// File: bilateral-filtering.frag.glsl

// Input texture to be filtered
uniform vec2 uInput_pixel_size;
uniform sampler2D uInput;

// Output texture filtered
uniform vec2 uOutput_pixel_size;
out vec3 uOutput;

// Filtering kernel textures used to guide the filtering
uniform bool uPosition_weight;
uniform float uPosition_sigma;
uniform sampler2D uPosition; // World space

uniform bool uNormal_weight;
uniform float uNormal_sigma;
uniform sampler2D uNormal;

// Normalmapping
uniform bool uNormal_mapping;
uniform sampler2D uTangent;
uniform sampler2D uTangent_normal;

uniform bool uDepth_weight;
uniform float uDepth_sigma;
uniform sampler2D uDepth;

// NOTE: Spatial kernel in texture space
#define MAX_KERNEL_ELEMENTS 15
uniform uint uKernel_dim; // Resulting kernel dim is x**2
// uniform float uKernel_sample_offsets[5] = {}; // TODO: Linear sampling
uniform float uKernel[MAX_KERNEL_ELEMENTS];

#ifdef HORIZONTAL_STEP_DIR
const vec2 STEP_DIR = vec2(1.0, 0.0);
#endif

#ifdef VERTICAL_STEP_DIR
const vec2 STEP_DIR = vec2(0.0, 1.0);
#endif

/// Computes the filtering weights for 'c' (center) and 'p' another pixel
float weights(const vec2 c, const vec2 p) {
  float w = 1.0;

  if (uPosition_weight) {
    const vec3 pos_c = texture(uPosition, c).xyz; // FIXME: Fetched more than once
    const vec3 pos_p = texture(uPosition, p).xyz;
    const vec3 dp = pos_c - pos_p;
    w *= gaussian(dot(dp, dp), uPosition_sigma);
  }

  if (uNormal_weight) {
    vec3 norm_c = texture(uNormal, c).xyz; // FIXME: Computed more than once
    vec3 norm_p = texture(uNormal, p).xyz; // FIXME: Computed more than once

    // Tangent normal mapping & TBN tranformation
    if (uNormal_mapping) {
      // Normal at c
      {
        const vec3 T = normalize(texture(uTangent, c).xyz);
        const vec3 B = normalize(cross(norm_c, T));
        const vec3 N = normalize(norm_c);
        const mat3 TBN = mat3(T, B, N);

        const vec3 tangent = texture(uTangent_normal, c).xyz;
        const vec3 TN = normalize(2.0 * tangent - vec3(0.5));

        // No tangent map data available
        if (dot(tangent, tangent) <= EPSILON) {
          norm_c = normalize(vec3(1.0));
        } else {
          norm_c = normalize(TBN * TN);
        }
      }

      // Normal at p
      {
        const vec3 T = normalize(texture(uTangent, p).xyz);
        const vec3 B = normalize(cross(norm_p, T));
        const vec3 N = normalize(norm_p);
        const mat3 TBN = mat3(T, B, N);

        const vec3 tangent = texture(uTangent_normal, p).xyz;
        const vec3 TN = normalize(2.0 * tangent - vec3(0.5));

        // No tangent map data available
        if (dot(tangent, tangent) <= EPSILON) {
          norm_p = normalize(vec3(1.0));
        } else {
          norm_p = normalize(TBN * TN);
        }
      }
    }

    const vec3 dn = norm_c - norm_p;
    w *= gaussian(dot(dn, dn), uNormal_sigma);
  }

  if (uDepth_weight) {
    const float depth_c = linearize_depth(texture(uDepth, c).r);  // FIXME: Fetched more than once
    const float depth_p = linearize_depth(texture(uDepth, p).r);
    const float dd = clamp(1.0 / (EPSILON + abs(depth_c - depth_p)), 0.0, 1.0);
    w *= gaussian(dd, uDepth_sigma);
  }

  return w;
}

// TODO: Normalmapping in filtering?
void main() {
  const vec2 c_i = gl_FragCoord.xy * uInput_pixel_size;
  const vec2 c_o = gl_FragCoord.xy * uOutput_pixel_size;

  const vec2 offset_i = STEP_DIR * uInput_pixel_size;
  const vec2 offset_o = STEP_DIR * uOutput_pixel_size;

  const uint R = uKernel_dim - 1; // Kernel radius

  float cum_w = 0.0; // Cumulative weight used for normalization

  const float w = uKernel[0] * weights(c_o, c_o);
  uOutput += w * texture(uInput, c_i).rgb;
  cum_w += w;

  for (uint i = 1; i <= R; i++) {
    const vec2 p_o = c_o - offset_o * i;
    const float w0 = uKernel[i] * weights(c_o, p_o);
    cum_w += w0;
    const vec2 p_i = c_i - offset_i * i;
    uOutput += w0 * texture(uInput, p_i).rgb;

    // --------------------------------------------------------------

    const vec2 q_o = c_o + offset_o * i;
    const float w1 = uKernel[i] * weights(c_o, q_o);
    cum_w += w1;
    const vec2 q_i = c_i + offset_i * i;
    uOutput += w1 * texture(uInput, q_i).rgb;
  }

  // Only normalize again if there are any other weights
  if (uPosition_weight || uNormal_weight || uDepth_weight) {
    uOutput /= cum_w;
  }
}
