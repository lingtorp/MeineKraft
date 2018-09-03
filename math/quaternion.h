#ifndef MEINEKRAFT_QUATERNION_H
#define MEINEKRAFT_QUATERNION_H

#include "vector.h"

struct quat {
  Vec3<float> v;
  float w;

  quat(): v(0.0), w(1.0) {}
  quat(Vec3<float> v): v(v), w(1.0) {}

  quat inverse() const {

  }



  /// Operators
};


#endif // MEINEKRAFT_QUATERNION_H