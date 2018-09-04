#ifndef MEINEKRAFT_QUATERNION_H
#define MEINEKRAFT_QUATERNION_H

#include "vector.h"

struct quat;

template<typename T>
inline quat operator*(const T s, const quat& v);

struct quat {
  Vec3<float> v;
  float w;

  explicit quat(): v(0.0), w(1.0) {}
  explicit quat(const Vec3<float>& v): v(v), w(1.0) {}
  quat(const Vec3<float>& v, const float w) : v(v), w(w) {}

  float norm() const {
    return std::sqrt(v.dot(v) + w * w);
  }

  quat inverse() const {
    return (1 / this->norm()) * this->conjugate();
  }

  quat conjugate() const {
    return quat{-v, w};
  }

  /// Utils
  /// Rotates point around the quat vector v by deg degrees
  Vec3<float> rotate(const Vec3<float>& point, const float rads) const {
    quat p(point, 1.0);
    quat q(std::sinf(rads / 2.0f) * v.normalize(), std::cosf(rads / 2.0f));
    return quat(q * (p * q.inverse())).v;
  }

  /// Operators
  inline quat operator*(const quat& r) {
    return quat{v.cross(r.v) + r.w * v + w * r.v, w * r.w - v.dot(r.v)};
  }

  friend std::ostream &operator<<(std::ostream& os, const quat& q) {
    return os << "(i:" << q.v.x << " j:" << q.v.y << " k:" << q.v.z << " w:" << q.w << ")" << std::endl;
  }
};

template<typename T>
inline quat operator*(const T s, const quat& q) {
  return quat(s * q.v, s * q.w);
}

template<typename T>
inline quat operator*(const quat& q, const T s) {
  return quat(s * q.v, s * q.w);
}

#endif // MEINEKRAFT_QUATERNION_H