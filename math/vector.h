#ifndef MEINEKRAFT_VECTOR_H
#define MEINEKRAFT_VECTOR_H

#include <SDL2/SDL_opengl.h>
#include <math.h>

struct Point3 { GLfloat x, y, z; };
typedef Point3 Vec3;

struct Point2 { GLfloat x, y; };
typedef Point2 Vec2;

static inline GLfloat dot(Vec3 v, Vec3 u) { return v.x * u.x + v.y * u.y + v.z * u.z; }

static inline Point2 new_point2(GLfloat x, GLfloat y) {
    Point2 point;
    point.x = x;
    point.y = y;
    return point;
}

static inline Point3 new_point3(GLfloat x, GLfloat y, GLfloat z) {
    Point3 point;
    point.x = x;
    point.y = y;
    point.z = z;
    return point;
}

static inline Vec3 normalize(Vec3 vec) {
    float length = sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
    Vec3 result;
    result.x = vec.x / length;
    result.y = vec.y / length;
    result.z = vec.z / length;
    return result;
}

static inline Vec3 vec_subtraction(Vec3 v, Vec3 u) {
    v.x -= u.x;
    v.y -= u.y;
    v.z -= u.z;
    return v;
}

static inline Vec3 vec_addition(Vec3 v, Vec3 u) {
    v.x += u.x;
    v.y += u.y;
    v.z += u.z;
    return v;
}

static inline Vec3 vec_scalar_multiplication(Vec3 v, float s) {
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return v;
}

// result = v x u
static inline Vec3 cross(Vec3 v, Vec3 u) {
    Vec3 result;
    result.x = v.y * u.z - v.z * u.y;
    result.y = v.z * u.x - v.x * u.z;
    result.z = v.x * u.y - v.y * u.x;
    return result;
}

static inline Vec3 zero_vec() {
    Vec3 result;
    result.x = 0.0f;
    result.y = 0.0f;
    result.z = 0.0f;
    return result;
}

#endif //MEINEKRAFT_VECTOR_H
