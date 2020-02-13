#pragma once
#ifndef MEINEKRAFT_LIGHT_HPP
#define MEINEKRAFT_LIGHT_HPP

#include "../math/vector.hpp"

/// Padded in order to fit with the shader declaration
struct PointLight {
  Vec4f position;   // (X, Y, X, padding)
  Vec4f intensity;  // (R, G, B, padding)

  explicit PointLight(const Vec3f& position): position(position), intensity(Vec4f(1.0f)) {};

  friend std::ostream &operator<<(std::ostream &os, const PointLight &light) {
    return os << "PointLight(position: " << light.position << ", intensity: " << light.intensity << ")";
  }
};

#endif // MEINEKRAFT_LIGHT_HPP
