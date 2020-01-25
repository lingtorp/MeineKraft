// NOTE: Bilinear upsampling of uInput texture

uniform sampler2D uInput;

uniform vec2 uOutput_pixel_size;
out vec3 uOutput;

void main() {
  const vec2 p = gl_FragCoord.xy * uOutput_pixel_size;
  uOutput = texture(uInput, p).rgb;
}
