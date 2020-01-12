// NOTE: Bilateral filtering downsampled cone traced image
// File: bf-rasterization.frag.glsl

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

// NOTE: Gaussian discretization (normalized) kernels from http://dev.theomader.com/gaussian-kernel-calculator/
// DIMENSION = 3
const int KERNEL_DIM = 3;

// SIGMA = 0.3
// const float kernel[9] = {
//     0.002284, 0.043222, 0.002284,
//     0.043222, 0.817976, 0.043222,
//     0.002284, 0.043222, 0.002284,
// };

// SIGMA = 0.6
// const float kernel[9] = {
//     0.039436, 0.119713, 0.039436,
//     0.119713, 0.363404, 0.119713,
//     0.039436, 0.119713, 0.039436,
// };

// SIGMA = 0.8
const float kernel[9] = {
    0.06292,  0.124998, 0.06292,
    0.124998, 0.248326, 0.124998,
    0.06292,  0.124998, 0.06292,
};

// FIXME: All dimension 5 matrices leads to garbage values in filtering ..
// DIMENSION = 5
// const int KERNEL_DIM = 5;

// SIGMA 0.8
// const float kernel_1d[5] = {0.02956, 0.236009, 0.468863, 0.236009, 0.02956};

// const float kernel[25] = {
//     0.000874, 0.006976, 0.01386,  0.006976, 0.000874,
//     0.006976, 0.0557,   0.110656, 0.0557,   0.006976,
//     0.01386,  0.110656, 0.219833, 0.110656, 0.01386,
//     0.006976, 0.0557,   0.110656, 0.0557,   0.006976,
//     0.000874, 0.006976, 0.01386,  0.006976, 0.000874,
// };

// SIGMA = 0.6
// const float kernel_1d[5] = {0.006194, 0.196125, 0.595362, 0.196125, 0.006194};
// const float kernel[25] = {
//     0.000038, 0.001215, 0.003688, 0.001215, 0.000038,
//     0.001215, 0.038465, 0.116765, 0.038465, 0.001215,
//     0.003688, 0.116765, 0.354456, 0.116765, 0.003688,
//     0.001215, 0.038465, 0.116765, 0.038465, 0.001215,
//     0.000038, 0.001215, 0.003688, 0.001215, 0.000038,
// };

void main() {
  const vec2 c = gl_FragCoord.xy * uPixel_size;

  const int R = KERNEL_DIM - 2; // Kernel radius
  for (int i = -R; i <= R; i++) {
    for (int j = -R; j <= R; j++) {
      const ivec2 wv = ivec2(R, R) + ivec2(i, j);
      const float w = kernel[wv.x * KERNEL_DIM + wv.y];
      const vec2 offset = vec2(i, j) * uPixel_size;
      uOutput += w * texture(uInput, c + offset).rgb;
    }
  }
}
