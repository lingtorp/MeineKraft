// NOTE: VCT utilities and functions
// File: voxel-cone-tracing-utils.glsl

// Number of clipmaps
#define NUM_CLIPMAPS 4

// Mathematical constants
const float M_PI = 3.1415926535897932384626433832795;
const float M_2PI = 2 * M_PI;
const float M_PI_2 = 1.57079632679489661923;
const float M_PI_4 = 0.785398163397448309616;

const float M_PI_INV = 1.0 / M_PI;
const float M_1_2PI = 1.0 / M_2PI;

// Material handling related stuff
const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

// Linear --> sRGB approximation
// See http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
// Src: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag
vec3 linear_to_sRGB(const vec3 color) {
  return pow(color, vec3(INV_GAMMA));
}

// sRGB to linear approximation
// See http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
// Src: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag
vec4 sRGB_to_linear(const vec4 sRGB) {
  return vec4(pow(sRGB.xyz, vec3(GAMMA)), sRGB.w);
}

// Returns true if point, p, is inside the AABB
bool is_inside_AABB(const vec3 aabb_min, const vec3 aabb_max, const vec3 p) {
  if (p.x > aabb_max.x || p.x < aabb_min.x) { return false; }
  if (p.y > aabb_max.y || p.y < aabb_min.y) { return false; }
  if (p.z > aabb_max.z || p.z < aabb_min.z) { return false; }
  return true;
}

// NOTE: Texture border extends additionally one HALF texel from the texture space (0,0) -- (1,1)
vec3 world_to_clipmap_voxelspace(const vec3  p,   // World position
                                 const float s,   // Scaling factor of the clipmap
                                 const vec3  c) { // AABB center of the clipmap
  return (p - c) * s + 0.5;
}
