// NOTE: Bilateral filtering downsampled cone traced image
// File: bf-rasterization.frag.glsl

layout(RGBA8) uniform restrict volatile readonly image2D uInput;
layout(RGBA8) uniform restrict volatile writeonly image2D uOutput;

void main() {
  const ivec2 p = pixel_to_texture_space(gl_FragCoord.xy);
  imageStore(uOutput, imageLoad(uInput, p).rgb);
}
