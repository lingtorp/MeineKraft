/// Boxdownsamples textures for use in bilateral upsampling
/// File: gbuffer-downsampled.frag.glsl

uniform vec2 uInput_pixel_size;

uniform sampler2D uPosition;
uniform sampler2D uNormal;
uniform sampler2D uDepth;

layout(location = 0) out vec3  gPosition;
layout(location = 1) out vec3  gNormal;
layout(location = 2) out float gDepth;

void main() {
  const vec2 c = gl_FragCoord.xy * uInput_pixel_size;
  gPosition = texture(uPosition, c).xyz;
  gNormal = texture(uNormal, c).xyz;
  gDepth = texture(uDepth, c).r;
}
