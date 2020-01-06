// NOTE: Performs bilateral filtering ...

// #version 450 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint uNth_pixel;

layout(RGBA8) uniform restrict volatile readonly image2D uInput;
layout(RGBA8) uniform restrict volatile writeonly image2D uOutput;

uniform uint uScreen_width;
uniform uint uScreen_height;

uniform uint uKernel_size; // Kernel box size

// 1D Gaussian function
// Src: https://people.csail.mit.edu/sparis/bf_course/course_notes.pdf
float gaussian(const float sigma, const float x) {
  const float factor = 1.0 / (M_2PI * sigma * sigma);
  return factor * exp(- x * x / (2.0 * sigma * sigma));
}

// Range (a.k.a domain/value) kernel function
float range(const float p, const float q) {
  const float d = p - q;
  return gaussian(0.1, d);
}

float spatial(const float p, const float q) {
  const float d = abs(p - q);
  return gaussian(0.1, d);
}

void main() {
  const vec2 screen_dims = vec2(uScreen_width, uScreen_height);
  const vec2 frag_coord = (uNth_pixel * gl_GlobalInvocationID.xy) / screen_dims;

  // Current pixel
  const vec3 p = imageLoad(uInput, ivec2(frag_coord * screen_dims));

  vec4 color = vec4(0.0);

  float intensity = 0.0;
  float weight = 0.0;

  for (uint i = -uKernel_size; i < uKernel_size; i++) {
    for (uint j = -uKernel_size; j < uKernel_size; j++) {

      const vec2 q = p + vec2(i, j); // Kernel space? Texture space not pixel space??
      const vec3 iq = 0.0; // intensity of pixel q
      const vec3 ip = imageLoad(uInput, ivec2(frag_coord * screen_dims));

      const float w = spatial(p, q) * range(ip, iq);

      color += vec4(vec3(w * iq), 0.0);
      weight += w;
    }
  }

  // Normalization
  color /= weight;

  imageStore(uOutput, ivec2(frag_coord * screen_dims), color);
}
