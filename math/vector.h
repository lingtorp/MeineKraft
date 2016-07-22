#ifndef MEINEKRAFT_VECTOR_H
#define MEINEKRAFT_VECTOR_H

#include <SDL2/SDL_opengl.h>
#include <math.h>
#include <ostream>

template<typename T>
struct Vec4 {
    T values[4];

    T& operator[] (const int index) {
        return values[index];
    }
};

template<typename T>
struct Mat4 {
    Vec4<T> rows[4];

    Vec4<T> &operator[] (const int index) {
        return rows[index];
    }

    inline T *data() {
        return &rows[0][0];
    }
};

struct Vec3 {
    GLfloat x, y, z = 0.0f;
    Vec3(GLfloat x, GLfloat y, GLfloat z): x(x), y(y), z(z) {};
    Vec3(): x(0), y(0), z(0) {};
    static Vec3 ZERO() { return Vec3(0.0, 0.0, 0.0); }

    bool operator==(const Vec3 &rhs) {
        return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
    }

    friend std::ostream &operator<<(std::ostream& os, const Vec3 &vec) {
        os << "(x:" << vec.x << " y:" << vec.y << " z:" << vec.z << ")";
        return os;
    }
};

struct Vec2 {
    GLfloat x, y = 0.0f;
    Vec2(GLfloat x, GLfloat y): x(x), y(y) {};
    Vec2(): x(0), y(0) {};
};

static inline GLfloat dot(Vec3 v, Vec3 u) { return v.x * u.x + v.y * u.y + v.z * u.z; }

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

static inline Vec3 vec_scalar_multiplication(Vec3 v, double s) {
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return v;
}

// result = v x u
static inline Vec3 cross(Vec3 v, Vec3 u) {
    Vec3 result = Vec3();
    result.x = v.y * u.z - v.z * u.y;
    result.y = v.z * u.x - v.x * u.z;
    result.z = v.x * u.y - v.y * u.x;
    return result;
}

#endif //MEINEKRAFT_VECTOR_H
