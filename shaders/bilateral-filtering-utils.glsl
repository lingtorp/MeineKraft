// NOTE: Bilateral filtering utility functions
// File: bilateral-filtering-utils.glsl
// [0]: https://bartwronski.com/2019/09/22/local-linear-models-guided-filter/

// --- Uniform Requirements ---
uniform uint uScreen_width;
uniform uint uScreen_height;

uniform float uSigmaSpatial; // Spatial kernel Gaussian sigma
uniform float uSigmaRange;   // Range kernel Gaussian sigma

// def T === ([0, 1], [0, 1]) <-- ([0, W], [0, H])
vec2 pixel_to_texture_space(const ivec2 p) {
  const vec2 screen_dims = vec2(uScreen_width, uScreen_height);
  return p / screen_dims;
}

// def T === ([0, 1], [0, 1]) --> ([0, W], [0, H])
ivec2 texture_to_pixel_space(const vec2 p) {
  const vec2 screen_dims = vec2(uScreen_width, uScreen_height);
  return ivec2(p * screen_dims);
}

// 1D Gaussian function
// Src: https://people.csail.mit.edu/sparis/bf_course/course_notes.pdf
float gaussian(const float x, const float sigma) {
  return exp(- 0.5 * x * x / (sigma * sigma));
}

// 1D Gaussian function
// Src: https://people.csail.mit.edu/sparis/bf_course/course_notes.pdf
float gaussian_1d(const float sigma, const float x) {
  const float factor = 1.0 / (M_2PI * sigma * sigma);
  return factor * exp(- x * x / (2.0 * sigma * sigma));
}

// Range (a.k.a domain/value) kernel function
// Best name for this function is the 'signal similarity' function[0]
// NOTE: Euclidean distance in RGB space
// TODO: Are there better ways to measure color difference?
float range(const vec3 p, const vec3 q) {
  const float d = distance(p, q);
  return gaussian_1d(uSigmaRange, d);
}

// Computes spatial likeness with Euclidean distance in texture space
// Input p, q are in kernel/image space
float spatial(const ivec2 p, const ivec2 q) {
  const vec2 tp = pixel_to_texture_space(p);
  const vec2 tq = pixel_to_texture_space(q);
  const float d = distance(tp, tq);
  return gaussian_1d(uSigmaSpatial, d);
}

// TODO: Euclidean distance in WORLD SPACE
// float spatial_ws(const ivec2p, const ivec2 q) {}
