// NOTE: Joint bilateral upsampling pass
// File: bf-separable.frag.glsl

// Low-res src texture
uniform vec2 uInput_pixel_size;
uniform sampler2D uInput;

// High-res result texture
uniform vec2 uOutput_pixel_size;
out vec3 uOutput;

// Guide textures (full resolution)
uniform bool uPosition_weight;
uniform float uPosition_sigma;
uniform sampler2D uPosition; // World space

uniform bool uNormal_weight;
uniform float uNormal_sigma;
uniform sampler2D uNormal;

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

// 'c' = pixel currently being shade, 'p' some other pixel
float weights(const vec2 c, const vec2 p) {
  float w = 1.0;

  if (uPosition_weight) {
    const vec3 pos_c = texture(uPosition, c).xyz;
    const vec3 pos_p = texture(uPosition, p).xyz;
    const vec3 dp = pos_c - pos_p;
    w *= gaussian(dot(dp, dp), uPosition_sigma);
  }

  if (uNormal_weight) {
    const vec3 norm_c = texture(uNormal, c).xyz;
    const vec3 norm_p = texture(uNormal, p).xyz;
    const vec3 dn = norm_c - norm_p;
    w *= gaussian(dot(dn, dn), uNormal_sigma);
  }

  return w;
}

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
  if (uPosition_weight || uNormal_weight) {
    uOutput /= cum_w;
  }
}
