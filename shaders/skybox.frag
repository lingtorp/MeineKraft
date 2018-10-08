
in vec3 fPosition;

uniform sampler2D equirectangular_environment;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 sample_sphericalmap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

out vec4 outColor; // Defaults to zero when the frag shader only has 1 out variable

void main() {
    outColor = vec4(texture(equirectangular_environment, sample_sphericalmap(normalize(fPosition))).rgb, 1.0);
}