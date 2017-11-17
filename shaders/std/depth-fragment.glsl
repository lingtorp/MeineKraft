out float outColor; // Defaults to zero when the frag shader only has 1 out variable

void main() {
    outColor = gl_FragCoord.z;
}