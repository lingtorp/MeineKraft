
uniform float uScreen_height;
uniform float uScreen_width;

uniform sampler2D uDiffuse;
// uniform sampler2D uNormal;
// uniform sampler2D uPosition; // World space

out vec4 color;

// TODO: Add tracing functions, materials, etc

void main() {
  const vec2 frag_coord = vec2(gl_FragCoord.x / uScreen_width, gl_FragCoord.y / uScreen_height);
  const vec3 diffuse = texture(uDiffuse, frag_coord).rgb;
  color = vec4(diffuse, 1.0f);
} 
