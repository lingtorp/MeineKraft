
uniform vec2 uInput_pixel_size;

uniform sampler2D uPosition;
uniform sampler2D uNormal;

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;

void main() {
  const vec2 c = gl_FragCoord.xy * uInput_pixel_size;
  gPosition = texture(uPosition, c).xyz;
  gNormal = texture(uNormal, c).xyz;
}
