// NOTE: Gaussian separable vertical blur pass
// File: bf-separable-horizontal.frag.glsl

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

const int KERNEL_DIM = 5;
// uniform float offsets[5] = {}; // Linear sampling
uniform float kernel[5] = {0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162};

void main() {
  const vec2 c = gl_FragCoord.xy * uPixel_size;

  const int R = KERNEL_DIM - 2; // Kernel radius

  uOutput += kernel[0] * texture(uInput, c).rgb;

  for (int i = 1; i < R; i++) {
    const vec2 offset = vec2(i, 0) * uPixel_size;
    uOutput += kernel[i] * texture(uInput, c - offset).rgb;
    uOutput += kernel[i] * texture(uInput, c + offset).rgb;
  }
}
