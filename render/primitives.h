#ifndef MEINEKRAFT_PRIMITIVES_H
#define MEINEKRAFT_PRIMITIVES_H

#include <cstdlib>
#include <vector>
#include "../math/vector.h"

/// Colors
template<typename T>
struct Color4 {
    T r, g, b, a = 0;
    Color4(): r(0), g(0), b(0), a(0) {};
    Color4(T r, T g, T b, T a): r(r), g(g), b(b), a(a) {};
    inline static Color4 clear()  { return Color4(0.0f, 0.0f, 0.0f, 0.0f); }
    inline static Color4 red()    { return Color4(1.0f, 0.0f, 0.0f, 1.0f); }
    inline static Color4 blue()   { return Color4(0.0f, 1.0f, 0.0f, 1.0f); }
    inline static Color4 green()  { return Color4(0.0f, 0.0f, 1.0f, 1.0f); }
    inline static Color4 yellow() { return Color4(1.0f, 1.0f, 0.0f, 1.0f); }
};

template<typename T>
struct Vertex {
    Vec3<T> position = {};
    Color4<T> color = {};
    Vec2<T> texCoord = {};
    Vertex(): position{}, color{}, texCoord{} {};
    Vertex(Vec3<T> position, Vec2<T> texCoord): position(position), texCoord(texCoord), color{} {};
    Vertex(Vec3<T> position, Color4<T> color, Vec2<T> texCoord):
            position(position), color(color), texCoord(texCoord) {};
};

struct Mesh {
    std::vector<Vertex<float>> vertices;
    std::vector<int> indices;

    Mesh(): vertices(std::vector<Vertex<float>>()), indices(std::vector<int>()) {};

    Mesh(std::vector<Vertex<float>> vertices, std::vector<int> indices):
            vertices(vertices), indices(indices) {};

    // 1 quad = 2 triangles => 4 vertices = 4 points & 4 colors
    /// Converts a quad to vertices
    std::vector<float> to_floats() {
        std::vector<float> floats;
        for (auto vertex : vertices) {
            floats.push_back(vertex.position.x);
            floats.push_back(vertex.position.y);
            floats.push_back(vertex.position.z);
            floats.push_back(vertex.color.r);
            floats.push_back(vertex.color.g);
            floats.push_back(vertex.color.b);
            floats.push_back(vertex.color.a);
            floats.push_back(vertex.texCoord.x);
            floats.push_back(vertex.texCoord.y);
        }
        return floats;
    }

    /// Byte size of vertices to upload to OpenGL
    size_t byte_size_of_vertices() {
        return sizeof(Vertex<float>) * vertices.size();
    }

    /// Byte size of indices to upload to OpenGL
    size_t byte_size_of_indices() {
        return sizeof(int) * indices.size();
    }
};

// Input: 4 points, 4 colors
//  d ----- c   <-- Point & color pairs
//  |       |
//  a ----- b   0 = a, 1 = b, 2 = c, 3 = d
struct Quad: Mesh {
    Quad(): Mesh() {};
    Quad(Vec3<float> points[4], Color4<float> colors[4], Vec2<float> texCoords[4]): Mesh() {
        vertices.push_back(Vertex<float>(points[0], colors[0], texCoords[0]));
        vertices.push_back(Vertex<float>(points[1], colors[1], texCoords[1]));
        vertices.push_back(Vertex<float>(points[2], colors[2], texCoords[2]));
        vertices.push_back(Vertex<float>(points[3], colors[3], texCoords[3]));
        indices.push_back(0); indices.push_back(1);
        indices.push_back(2); indices.push_back(3);
    };
};

enum Texture: uint64_t { SKYBOX, DIRT, GRASS, AIR };

struct Cube: Mesh {
    Vec3<float> position; // Lower left corner of the cube in world space
    float theta_x, theta_y, theta_z; // Rotation in object space
    float scale;
    Texture texture;

    // TODO: Refactor later - used for Path tracing
    Vec3<float> center;
    double radius;

    Cube(): Mesh(), scale(1.0), texture(Texture::GRASS), position(Vec3<float>::ZERO()),
            theta_x(0.0), theta_y(0.0), theta_z(0.0), center(Vec3<float>::ZERO()), radius(0) {
        auto a = Vec3<float>(-0.5f, -0.5f, 0.5f);
        auto b = Vec3<float>(0.5f, -0.5f, 0.5f);
        auto c = Vec3<float>(0.5f, 0.5f, 0.5f);
        auto d = Vec3<float>(-0.5f, 0.5f, 0.5f);
        auto tex_a = Vec2<float>(0.0f, 0.0f);
        auto tex_b = Vec2<float>(1.0f, 0.0f);
        auto tex_c = Vec2<float>(1.0f, 1.0f);
        auto tex_d = Vec2<float>(0.0f, 1.0f);
        vertices.push_back(Vertex<float>(a, tex_a));
        vertices.push_back(Vertex<float>(b, tex_b));
        vertices.push_back(Vertex<float>(c, tex_c));
        vertices.push_back(Vertex<float>(d, tex_d));

        auto e = Vec3<float>(-0.5f, -0.5f, -0.5f);
        auto f = Vec3<float>(0.5f, -0.5f, -0.5f);
        auto g = Vec3<float>(0.5f, 0.5f, -0.5f);
        auto h = Vec3<float>(-0.5f, 0.5f, -0.5f);
        auto tex_e = Vec2<float>(1.0f, 0.0f);
        auto tex_f = Vec2<float>(0.0f, 0.0f);
        auto tex_g = Vec2<float>(0.0f, 1.0f);
        auto tex_h = Vec2<float>(1.0f, 1.0f);
        vertices.push_back(Vertex<float>(e, tex_e));
        vertices.push_back(Vertex<float>(f, tex_f));
        vertices.push_back(Vertex<float>(g, tex_g));
        vertices.push_back(Vertex<float>(h, tex_h));

        indices =  { // front
                    0, 1, 2, 2, 3, 0,
                    // right
                    1, 5, 6, 6, 2, 1,
                    // back
                    7, 6, 5, 5, 4, 7,
                    // left
                    4, 0, 3, 3, 7, 4,
                    // bot
                    4, 5, 1, 1, 0, 4,
                    // top
                    3, 2, 6, 6, 7, 3};
    }
};

/// Mathematical plane: a*x + b*y + c*z = d
template<typename T>
struct Plane {
    T a, b, c, d;

    Plane(): a(0), b(0), c(0), d(0) {};
    Plane(T a, T b, T c, T d): a(a), b(b), c(c), d(d) {};

    /// Normal of the plane
    /// @return Normal vector of the plane
    Vec3<T> normal(bool normalized) {
        return Vec3<T>(a, b, c);
    }

    /// Normalizes the plane
    /// @return Normalized plane (self)
    Plane<T> normalize() {
        double mag = sqrt(a * a + b * b + c * c);
        a = a / mag;
        b = b / mag;
        c = c / mag;
        d = d / mag;
        return *this;
    }

    /// Distance to point from the plane
    /// distance < 0, then point lies in the negative halfspace
    /// distance = 0, then point lies in the plane
    /// distance > 0, then point lies in the positive halfspace
    double distance_to_point(const Vec3<T> point) {
        return a*point.x + b*point.y + c*point.z + d;
    }
};

#endif //MEINEKRAFT_PRIMITIVES_H
