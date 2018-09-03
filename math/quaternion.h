#ifndef MEINEKRAFT_QUATERNION_H
#define MEINEKRAFT_QUATERNION_H

#include "vector.h"

struct quat {
  Vec3<float> v;
  float w;

  quat(): v(0.0), w(1.0) {}
  quat(const Vec3<float>& v): v(v), w(1.0) {}

  float norm() const {
    return std::sqrt(v.dot(v) + w * w);
  }

  quat inverse() const {
    return (1 / this->norm()) * this->conjugate();
  }

  quat conjugate() const {
    return {-v, w};
  }

  /// Utils
  Mat4<float> matrix() const {
    return {};
  }

  /// Rotates point around the quat vector v by deg degrees
  Vec3<float> rotate(const Vec3<float>& point, const float deg) const {
    quat p(point, 1.0);
    quat q(std::sinf(deg / 2.0f) * v.normalize(), std::cosf(deg / 2.0f));
    return q * (p * q.inverse());
  }

  /// Operators
  inline quat operator*(const quat& r) {
    return {v.cross(r.v) + r.w * v + w * r.v, w * r.w - v.dot(r.v)};
  }

  inline quat operator*(const float s) {
    return {s * v, s * w};
  }
};

#endif // MEINEKRAFT_QUATERNION_H