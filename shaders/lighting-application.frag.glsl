
// Material colors
uniform sampler2D uDiffuse;

// Radiance textures
uniform sampler2D uIndirect_radiance; // RGB (float)
uniform sampler2D uEmissive_radiance; // RGB (float)
uniform sampler2D uSpecular_radiance; // RGB (float)
uniform sampler2D uAmbient_radiance;  // R   (float)
uniform sampler2D uDirect_radiance;   // RGB (float)

uniform vec2 uPixel_size;

out vec3 color;

void main() {
  const vec2 c = gl_FragCoord.xy * uPixel_size;

  // Indirect
  color += texture(uIndirect_radiance, c).rgb;

  // Emissive
  color += texture(uEmissive_radiance, c).rgb;

  // Specular
  color += texture(uSpecular_radiance, c).rgb;

  // Ambient
  color += vec3(texture(uAmbient_radiance, c).r);

  // Direct
  color += texture(uDirect_radiance, c).rgb;

  // TODO: Tonemapping here?

  // const vec3 diffuse = sRGB_to_linear(vec4(texture(uDiffuse, c).rgb, 1.0)).rgb;
  const vec3 diffuse = texture(uDiffuse, c).rgb;
  color *= diffuse;
}
