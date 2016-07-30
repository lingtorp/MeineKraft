#ifndef MEINEKRAFT_VECTOR_H
#define MEINEKRAFT_VECTOR_H

#include <SDL2/SDL_opengl.h>
#include <math.h>
#include <ostream>

template<typename T>
struct Vec4 {
    T values[4];

    // Operators
    T& operator[] (const int index) {
        return values[index];
    }

    bool operator==(const Vec4 &rhs) {
        return (values[0] == rhs.values[0]) && (values[1] == rhs.values[1]) && (values[2] == rhs.values[2]) && (values[3] == rhs.values[3]);
    }

    friend std::ostream &operator<<(std::ostream &os, const Vec4 &vec) {
        return os << "(x:" << vec.values[0] << " y:" << vec.values[1] << " z:" << vec.values[2] << " w:" << vec.values[3];
    }
};

// TODO: Make templated?
struct Vec3 {
    GLfloat x, y, z = 0.0f;
    Vec3(GLfloat x, GLfloat y, GLfloat z): x(x), y(y), z(z) {};
    Vec3(): x(0), y(0), z(0) {};
    static Vec3 ZERO() { return Vec3(0.0, 0.0, 0.0); }

    bool operator==(const Vec3 &rhs) {
        return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
    }

    friend std::ostream &operator<<(std::ostream& os, const Vec3 &vec) {
        return os << "(x:" << vec.x << " y:" << vec.y << " z:" << vec.z << ")";
    }
};

// TODO: Make templated?
struct Vec2 {
    GLfloat x, y = 0.0f;
    Vec2(GLfloat x, GLfloat y): x(x), y(y) {};
    Vec2(): x(0), y(0) {};
};

struct Vec22 {
    double x, y = 0.0f;
    Vec22(double x, double y): x(x), y(y) {};
    Vec22(): x(0), y(0) {};
};

template<typename T>
struct Mat4 {
    Vec4<T> rows[4];

    inline T *data() {
        return &rows[0][0];
    }

    /// Identity matrix by default
    Mat4<T>() {
        rows[0] = Vec4<T>{1.0f, 0.0f, 0.0f, 0.0f};
        rows[1] = Vec4<T>{0.0f, 1.0f, 0.0f, 0.0f};
        rows[2] = Vec4<T>{0.0f, 0.0f, 1.0f, 0.0f};
        rows[3] = Vec4<T>{0.0f, 0.0f, 0.0f, 1.0f};
    }

    /// Translation - moves the matrix projection in space ...
    Mat4<T> translate(Vec3 vec) const {
        Mat4<T> matrix;
        matrix[0] = {1.0f, 0.0f, 0.0f, vec.x};
        matrix[1] = {0.0f, 1.0f, 0.0f, vec.y};
        matrix[2] = {0.0f, 0.0f, 1.0f, vec.z};
        matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
        return *this * matrix;
    }

    /// Scales the matrix the same over all axis except w
    Mat4<T> scale(T scale) const {
        Mat4<T> matrix;
        matrix[0] = {scale, 0.0f, 0.0f, 0.0f};
        matrix[1] = {0.0f, scale, 0.0f, 0.0f};
        matrix[2] = {0.0f, 0.0f, scale, 0.0f};
        matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
        return *this * matrix;
    }

    /// Transposes the current matrix and returns that matrix
    Mat4<T> transpose() {
        Mat4<T> mat;
        mat[0][0] = rows[0][0];
        mat[1][1] = rows[1][1];
        mat[2][2] = rows[2][2];
        mat[3][3] = rows[3][3];

        mat[1][0] = rows[0][1];
        mat[2][0] = rows[0][2];
        mat[3][0] = rows[0][3];
        mat[0][1] = rows[1][0];
        mat[0][2] = rows[2][0];
        mat[0][3] = rows[3][0];

        mat[2][1] = rows[1][2];
        mat[3][1] = rows[1][3];
        mat[1][2] = rows[2][1];
        mat[1][3] = rows[3][1];

        mat[2][3] = rows[3][2];
        mat[3][2] = rows[2][3];
        return mat;
    }

    // Operators
    /// Standard matrix multiplication row-column wise; *this * mat
    Mat4<T> operator*(Mat4<T> mat) const {
        Mat4<T> matrix;
        for (int i = 0; i < 4; ++i) {
            auto row = rows[i];
            for (int j = 0; j < 4; ++j) {
                Vec4<T> column = Vec4<T>{mat[0][j], mat[1][j], mat[2][j], mat[3][j]};
                matrix.rows[i][j] = row[0]*column[0] + row[1]*column[1] + row[2]*column[2] + row[3]*column[3];
            }
        }
        return matrix;
    }

    Vec4<T> &operator[](const int index) {
        return rows[index];
    }

    friend std::ostream &operator<<(std::ostream& os, const Mat4 &mat) {
        return os << "\n { \n" << mat.rows[0] << "), \n" << mat.rows[1] << "), \n" << mat.rows[2] << "), \n" << mat.rows[3] << ")\n }";
    }
};

static inline GLfloat dot(Vec3 v, Vec3 u) { return v.x * u.x + v.y * u.y + v.z * u.z; }
static inline GLfloat dot(Vec2 v, Vec2 u) { return v.x * u.x + v.y * u.y; }

static inline double dot(Vec22 v, Vec22 u) { return v.x * u.x + v.y * u.y; }

static inline Vec22 normalize(Vec22 vec) {
    double length = sqrt(pow(vec.x, 2) + pow(vec.y, 2));
    Vec22 result;
    result.x = vec.x / length;
    result.y = vec.y / length;
    return result;
}

static inline Vec3 normalize(Vec3 vec) {
    float length = sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
    Vec3 result;
    result.x = vec.x / length;
    result.y = vec.y / length;
    result.z = vec.z / length;
    return result;
}

// result = v x u
static inline Vec3 cross(Vec3 v, Vec3 u) {
    Vec3 result = Vec3();
    result.x = v.y * u.z - v.z * u.y;
    result.y = v.z * u.x - v.x * u.z;
    result.z = v.x * u.y - v.y * u.x;
    return result;
}

// TODO: Move these into operators on their structs
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

#endif //MEINEKRAFT_VECTOR_H
