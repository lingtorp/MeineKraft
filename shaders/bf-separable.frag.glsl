// NOTE: Gaussian separable blur pass
// File: bf-separable.frag.glsl

// Low-res texture
uniform sampler2D uInput;

// Guide textures
uniform sampler2D uPosition; // World space
uniform sampler2D uNormal;

layout(location = 0) out vec3 uOutput;

uniform vec2 uPixel_size;

// NOTE: Uses multiple edge-stopping functions that takes into
// account ray-traced, normal, position buffer.
// Title: Edge-Avoiding À-Trous Wavelet Transform for fast GI Filtering
// https://jo.dreggn.org/home/2010_atrous.pdf

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

void main() {
  const vec2 c = gl_FragCoord.xy * uPixel_size;

  const uint R = uKernel_dim - 1; // Kernel radius

  uOutput += uKernel[0] * texture(uInput, c).rgb;

  for (uint i = 1; i <= R; i++) {
    const vec2 offset = STEP_DIR * uPixel_size;
    uOutput += uKernel[i] * texture(uInput, c - offset).rgb;
    uOutput += uKernel[i] * texture(uInput, c + offset).rgb;
  }
}
