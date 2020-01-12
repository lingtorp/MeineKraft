// NOTE: Bilateral filtering downsampled cone traced image
// File: bf-rasterization.frag.glsl

// Low-res texture
uniform sampler2D uInput;

// Guide textures
uniform sampler2D uPosition; // World space
uniform sampler2D uNormal;

layout(location = 0) out vec3 uOutput;

// NOTE: Uses multiple edge-stopping functions that takes into
// account ray-traced, normal, position buffer.
// Title: Edge-Avoiding À-Trous Wavelet Transform for fast GI Filtering
// https://jo.dreggn.org/home/2010_atrous.pdf

void main() {
  const vec2 p = vec2(gl_FragCoord.x / float(uScreen_width), gl_FragCoord.y / float(uScreen_height));

  // TODO: How do I sample the image
  // float value = 0.0;
  // const float sigma = 0.2; // Range: p - q <= 2.0 * sigma
  // for (float x0 = -sigma; x0 < sigma; x0 += ?) {
  //   for (float y0 = -sigma; y0 < sigma; y0 += ?) {
  //     const float w = ?;
  //     value += w * ?;
  //     weight += w;
  //   }
  // }

  // // Normalize
  // value /= weight;

  // uOutput = value;
  uOutput = texture(uInput, p).rgb;
}
