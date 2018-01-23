uniform sampler2D input_sampler;

uniform int blur_size; // Size of noise texture
uniform float blur_factor; // Blur diminishing factor

out float fResult;

void main() {
    vec2 frag_coord = vec2(gl_FragCoord.x / 1280.0, gl_FragCoord.y / 720.0);
    vec2 texel_size = 1.0 / vec2(textureSize(input_sampler, 0));
    float result = 0.0;
    for (int x = -blur_size; x < blur_size; x++) {
        for (int y = -blur_size; y < blur_size; y++) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(input_sampler, frag_coord + offset).r;
        }
    }
    fResult = result / blur_factor;
}