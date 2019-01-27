#pragma once
#ifndef MEINEKRAFT_QUATERNION_H
#define MEINEKRAFT_QUATERNION_H

#include "vector.h"

#include <glm/glm.hpp>

struct quat;
inline quat operator*(const float s, const quat& q);
inline quat operator*(const quat& q, const float s);

struct quat {
  Vec3f v;
  float w;

  explicit quat() : v(0.0f), w(1.0f) {}
  explicit quat(const Vec3f& v) : v(v), w(1.0f) {}
  quat(const Vec3f& v, const float w) : v(v), w(w) {}

  static inline quat X(const float rads) {
    return quat(std::sin(rads / 2.0f) * Vec3f(1.0f, 0.0f, 0.0f), std::cos(rads / 2.0f));
  }

  static inline quat Y(const float rads) {
    return quat(std::sin(rads / 2.0f) * Vec3f(0.0f, 1.0f, 0.0f), std::cos(rads / 2.0f));
  }

  static inline quat Z(const float rads) {
    return quat(std::sin(rads / 2.0f) * Vec3f(0.0f, 0.0f, 1.0f), std::cos(rads / 2.0f));
  }

  inline float norm() const {
    return std::sqrt(v.dot(v) + w * w);
  }

  inline quat inverse() const {
    return (1.0f / norm()) * conjugate();
  }

  inline quat conjugate() const {
    return quat{ -v, w };
  }

  /// Rotates point/vector around the vector v of the quat by rads radians
  inline Vec3f rotate(const Vec3f& v, const float rads) const {
    quat p(v, 1.0f);
    quat q(std::sin(rads / 2.0f) * v.normalize(), std::cos(rads / 2.0f));
    return quat(q * (p * q.inverse())).v;
  }

  inline quat operator*(const quat& r) {
    return quat(v.cross(r.v) + r.w * v + w * r.v, w * r.w - v.dot(r.v));
  }

  inline Mat4f to_matrix() const {
    const float s = 2.0 / (norm() * norm());
    Mat4f mat;
    mat[0] = { 1.0f - s * (v.y * v.y + v.z * v.z), s * (v.x * v.y - w * v.z), s * (v.x * v.z + w * v.y), 0.0f };
    mat[1] = { s * (v.x * v.y + w * v.z), 1.0f - s * (v.x * v.x + v.z * v.z), s * (v.y * v.z - w * v.x), 0.0f };
    mat[2] = { s * (v.x * v.z - w * v.y), s * (v.y * v.z - w * v.x), 1.0f - s * (v.x * v.x + v.y * v.y), 0.0f };
    mat[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
    return mat;
  }

  friend std::ostream &operator<<(std::ostream& os, const quat& q) {
    return os << "(i:" << q.v.x << " j:" << q.v.y << " k:" << q.v.z << " w:" << q.w << ")" << std::endl;
  }
};

// Rotations matrix with rotation (dx, dy, dz) = r degrees around each axis
inline Mat4f rotate(const Vec3f& r) {
  quat q = quat::X(glm::radians(r.x)) * quat::Y(glm::radians(r.y)) * quat::Z(glm::radians(r.z));
  return q.to_matrix();
}

inline quat operator*(const float s, const quat& q) {
  return quat(s * q.v, s * q.w);
}

inline quat operator*(const quat& q, const float s) {
  return quat(s * q.v, s * q.w);
}

#endif // MEINEKRAFT_QUATERNION_H

