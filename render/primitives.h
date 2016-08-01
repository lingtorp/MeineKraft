#ifndef MEINEKRAFT_PRIMITIVES_H
#define MEINEKRAFT_PRIMITIVES_H

#include <SDL_opengl.h>
#include <memory>
#include <cstdlib>
#include <vector>
#include "../math/vector.h"

// Colors
struct Color4 {
    GLfloat r, g, b, a = 0;
    Color4(): r(0), g(0), b(0), a(0) {};
    Color4(GLfloat r, GLfloat g, GLfloat b, GLfloat a): r(r), g(g), b(b), a(a) {};
    inline static Color4 clear()  { return Color4(0.0f, 0.0f, 0.0f, 0.0f); }
    inline static Color4 red()    { return Color4(1.0f, 0.0f, 0.0f, 1.0f); }
    inline static Color4 blue()   { return Color4(0.0f, 1.0f, 0.0f, 1.0f); }
    inline static Color4 green()  { return Color4(0.0f, 0.0f, 1.0f, 1.0f); }
    inline static Color4 yellow() { return Color4(1.0f, 1.0f, 0.0f, 1.0f); }
};

struct Vertex {
    Vec3 position = {};
    Color4 color = {};
    Vec2<GLfloat> texCoord = {};
    Vertex(): position{}, color{}, texCoord{} {};
    Vertex(Vec3 position, Vec2<GLfloat> texCoord): position(position), texCoord(texCoord), color{} {};
    Vertex(Vec3 position, Color4 color, Vec2<GLfloat> texCoord):
            position(position), color(color), texCoord(texCoord) {};
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    Mesh(): vertices(std::vector<Vertex>()), indices(std::vector<GLuint>()) {};

    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices):
            vertices(vertices), indices(indices) {};

    // Converts a quad to vertices
    // 1 quad = 2 triangles => 4 vertices = 4 points & 4 colors
    std::vector<GLfloat> to_floats() {
        std::vector<GLfloat> floats;
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
        return sizeof(Vertex) * vertices.size();
    }

    /// Byte size of indices to upload to OpenGL
    size_t byte_size_of_indices() {
        return sizeof(GLuint) * indices.size();
    }
};

// Input: 4 points, 4 colors
//  d ----- c   <-- Point & color pairs
//  |       |
//  a ----- b   0 = a, 1 = b, 2 = c, 3 = d
struct Quad: Mesh {
    Quad(): Mesh() {};
    Quad(Vec3 points[4], Color4 colors[4], Vec2<GLfloat> texCoords[4]): Mesh() {
        vertices.push_back(Vertex(points[0], colors[0], texCoords[0]));
        vertices.push_back(Vertex(points[1], colors[1], texCoords[1]));
        vertices.push_back(Vertex(points[2], colors[2], texCoords[2]));
        vertices.push_back(Vertex(points[3], colors[3], texCoords[3]));
        indices.push_back(0); indices.push_back(1);
        indices.push_back(2); indices.push_back(3);
    };
};

typedef enum { SKYBOX, DIRT, GRASS, AIR } Texture;

struct Cube: Mesh {
    Vec3 position; // Lower left corner of the cube in world space
    GLfloat theta_x, theta_y, theta_z; // Rotation in object space
    GLfloat scale;
    Texture texture;

    Cube(): Mesh(), scale(1.0), texture(Texture::GRASS), position(Vec3::ZERO()),
            theta_x(0.0), theta_y(0.0), theta_z(0.0) {
        Vec3 a = Vec3(-0.5f, -0.5f, 0.5f);
        Vec3 b = Vec3(0.5f, -0.5f, 0.5f);
        Vec3 c = Vec3(0.5f, 0.5f, 0.5f);
        Vec3 d = Vec3(-0.5f, 0.5f, 0.5f);
        auto tex_a = Vec2<GLfloat>(0.0f, 0.0f);
        auto tex_b = Vec2<GLfloat>(1.0f, 0.0f);
        auto tex_c = Vec2<GLfloat>(1.0f, 1.0f);
        auto tex_d = Vec2<GLfloat>(0.0f, 1.0f);
        vertices.push_back(Vertex(a, tex_a));
        vertices.push_back(Vertex(b, tex_b));
        vertices.push_back(Vertex(c, tex_c));
        vertices.push_back(Vertex(d, tex_d));

        Vec3 e = Vec3(-0.5f, -0.5f, -0.5f);
        Vec3 f = Vec3(0.5f, -0.5f, -0.5f);
        Vec3 g = Vec3(0.5f, 0.5f, -0.5f);
        Vec3 h = Vec3(-0.5f, 0.5f, -0.5f);
        auto tex_e = Vec2<GLfloat>(1.0f, 0.0f);
        auto tex_f = Vec2<GLfloat>(0.0f, 0.0f);
        auto tex_g = Vec2<GLfloat>(0.0f, 1.0f);
        auto tex_h = Vec2<GLfloat>(1.0f, 1.0f);
        vertices.push_back(Vertex(e, tex_e));
        vertices.push_back(Vertex(f, tex_f));
        vertices.push_back(Vertex(g, tex_g));
        vertices.push_back(Vertex(h, tex_h));

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

#endif //MEINEKRAFT_PRIMITIVES_H
