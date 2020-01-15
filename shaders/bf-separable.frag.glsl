// NOTE: Gaussian separable blur pass
// File: bf-separable.frag.glsl

// Low-res texture
uniform sampler2D uInput;
out vec3 uOutput;

// Guide textures
uniform bool uPosition_weight;
uniform float uPosition_sigma;
uniform sampler2D uPosition; // World space

uniform bool uNormal_weight;
uniform float uNormal_sigma;
uniform sampler2D uNormal;

uniform vec2 uPixel_size;

// NOTE: Uses multiple edge-stopping functions that takes into
// account ray-traced, normal, position buffer.
// Title: Edge-Avoiding À-Trous Wavelet Transform for fast GI Filtering
// https://jo.dreggn.org/home/2010_atrous.pdf

// NOTE: Spatial kernel in texture space
#define MAX_KERNEL_ELEMENTS 10
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
  const vec2 c = gl_FragCoord.xy * uPixel_size;

  const vec2 offset = STEP_DIR * uPixel_size;

  const uint R = uKernel_dim - 1; // Kernel radius

  float cum_w = 0.0; // Cumulative weight used for normalization

  const float w = uKernel[0] * weights(c, c);
  uOutput += w * texture(uInput, c).rgb;
  cum_w += w;

  for (uint i = 1; i <= R; i++) {
    const vec2 p = c - offset * i;
    const float w0 = uKernel[i] * weights(c, p);
    cum_w += w0;
    uOutput += w0 * texture(uInput, p).rgb;

    // --------------------------------------------------------------

    const vec2 q = c + offset * i;
    const float w1 = uKernel[i] * weights(c, q);
    cum_w += w1;
    uOutput += w1 * texture(uInput, q).rgb;
  }

  // Only normalize again if there are any other weights
  if (uPosition_weight || uNormal_weight) {
    uOutput /= cum_w;
  }
}
