#pragma once
#ifndef MEINEKRAFT_LIGHT_HPP
#define MEINEKRAFT_LIGHT_HPP

#include "../math/vector.hpp"

/// Padded in order to fit with the shader declaration
struct PointLight {
  Vec4f position;   // (X, Y, X, padding)
  Vec4f intensity;  // (R, G, B, padding)

  explicit PointLight(Vec3f position): position(position), intensity(230.47f, 210.31f, 200.79f) {};

  friend std::ostream &operator<<(std::ostream &os, const PointLight &light) { return os << light.position; }
};

#endif // MEINEKRAFT_LIGHT_HPP
