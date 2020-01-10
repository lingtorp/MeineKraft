// NOTE: Performs bilateral filtering using compute

// #version 450 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint uNth_pixel;

layout(RGBA8) uniform restrict volatile readonly image2D uInput;
layout(RGBA8) uniform restrict volatile writeonly image2D uOutput;

// Bilateral filtering compute implementation related
uniform uint uKernel_size;   // Kernel box size

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
