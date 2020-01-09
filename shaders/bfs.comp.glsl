// NOTE: Performs bilateral filtering ...

// #version 450 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint uNth_pixel;

layout(RGBA8) uniform restrict volatile readonly image2D uInput;
layout(RGBA8) uniform restrict volatile writeonly image2D uOutput;

uniform uint uScreen_width;
uniform uint uScreen_height;

// Bilateral filtering related
uniform uint uKernel_size;   // Kernel box size
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
float gaussian_1d(const float sigma, const float x) {
  const float factor = 1.0 / (M_2PI * sigma * sigma);
  return factor * exp(- x * x / (2.0 * sigma * sigma));
}

// Range (a.k.a domain/value) kernel function
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

// FIXME: Something is wrong in the way the kernel reads from the input texture
// imageLoad(uInput, p + ivec2(1,0)) does not seem to return the rightmost pixel value
void main() {
  vec4 color = vec4(0.0);

  // Current pixel
  const ivec2 p = ivec2(uNth_pixel * gl_GlobalInvocationID.xy);
  const vec3 ip = imageLoad(uInput, p).rgb;

  float intensity = 0.0;
  float weight = 0.0;

  // TODO: const vec2 texture_scale = 1.0 / vec2(uScreen_width, uScreen_height);
  // TODO: const float kernel_radius = 2.0 * uSigmaSpatial; // Texture space

  // Box kernel domain
  for (uint i = -uKernel_size / 2; i < uKernel_size / 2; i++) {
    for (uint j = -uKernel_size / 2; j < uKernel_size / 2; j++) {
      // Kernel space kernel
      const ivec2 q = p + ivec2(i, j);
      const vec3 iq = imageLoad(uInput, q).rgb;
      const float w = spatial(p, q) * range(ip, iq);
      color.rgb += w * iq;
      weight += w;
    }
  }

  // Normalization
  color /= weight;

  imageStore(uOutput, p, color);
}
