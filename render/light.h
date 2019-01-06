#pragma once
#ifndef MEINEKRAFT_LIGHT_H
#define MEINEKRAFT_LIGHT_H

#include "../math/vector.h"

/// Padded in order to fit with the shader declaration
struct PointLight {
  Vec4f position;   // (X, Y, X, padding)
  Vec4f intensity;  // (R, G, B, padding)

  explicit PointLight(Vec3f position): position(position), intensity(23.47, 21.31, 20.79) {};

  friend std::ostream &operator<<(std::ostream &os, const PointLight &light) { return os << light.position; }
};

#endif // MEINEKRAFT_LIGHT_H
