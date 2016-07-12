#ifndef MEINEKRAFT_PRIMITIVES_H
#define MEINEKRAFT_PRIMITIVES_H

#include <SDL_opengl.h>
#include <memory>
#include <cstdlib>
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
    Point3 position = {};
    Color4 color = {};
    Point2 texCoord = {};
    Vertex(): position{}, color{}, texCoord{} {};
    Vertex(Point3 position, Color4 color, Point2 texCoord):
            position(position), color(color), texCoord(texCoord) {};
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<int8_t> indices;

    Mesh(): vertices(std::vector<Vertex>()), indices(std::vector<int8_t>()) { };

    Mesh(std::vector<Vertex> vertices, std::vector<int8_t> indices):
            vertices(vertices), indices(indices) {};

    // Converts a quad to vertices
    // 1 quad = 2 triangles => 4 vertices = 4 points & 4 colors
    std::vector<GLfloat> to_floats() {
        std::vector<GLfloat> floats;
        int j = 0;
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
            j++;
        }
        return floats;
    }

    /// OpenGL size of vertices uploaded to OpenGL
    size_t size_of_vertices() {
        size_t size = 0;
        for (auto vertex : vertices) {
            size += sizeof(vertex);
        }
        return size;
    }
};

// Input: 4 points, 4 colors
//  d ----- c   <-- Point & color pairs
//  |       |
//  a ----- b   0 = a, 1 = b, 2 = c, 3 = d
struct Quad: Mesh {
    Quad(): Mesh() {};
    Quad(Point3 points[4], Color4 colors[4], Point2 texCoords[4]): Mesh() {
        vertices.push_back(Vertex(points[0], colors[0], texCoords[0]));
        vertices.push_back(Vertex(points[1], colors[1], texCoords[1]));
        vertices.push_back(Vertex(points[2], colors[2], texCoords[2]));
        vertices.push_back(Vertex(points[3], colors[3], texCoords[3]));
        indices.push_back(0); indices.push_back(1);
        indices.push_back(2); indices.push_back(3);
    };
};

typedef enum { SKYBOX, DIRT, GRASS, AIR } Texture;

struct Cube {
    Quad quads[2];
    Point3 position; // Center point of the cube
    GLfloat scale;
    GLfloat theta_x, theta_y, theta_z; // Rotation in object space
    Texture texture;

    Cube(): quads{Quad(), Quad()} {};

    Cube(Color4 *colors): quads{} {
        scale = 1.0f;
        texture = Texture::GRASS;
        Point3 a = Point3(-0.5f, -0.5f, 0.5f);
        Point3 b = Point3(0.5f, -0.5f, 0.5f);
        Point3 c = Point3(0.5f, 0.5f, 0.5f);
        Point3 d = Point3(-0.5f, 0.5f, 0.5f);
        Point3 vertices1[] = {a, b, c, d};
        Point2 tex_a = Point2(0.0f, 0.0f);
        Point2 tex_b = Point2(1.0f, 0.0f);
        Point2 tex_c = Point2(1.0f, 1.0f);
        Point2 tex_d = Point2(0.0f, 1.0f);
        Point2 texCoords1[] = {tex_a, tex_b, tex_c, tex_d};
        quads[0] = Quad(vertices1, colors, texCoords1);

        Point3 e = Point3(-0.5f, -0.5f, -0.5f);
        Point3 f = Point3(0.5f, -0.5f, -0.5f);
        Point3 g = Point3(0.5f, 0.5f, -0.5f);
        Point3 h = Point3(-0.5f, 0.5f, -0.5f);
        Point3 vertices2[] = {e, f, g, h};
        Point2 tex_e = Point2(1.0f, 0.0f);
        Point2 tex_f = Point2(0.0f, 0.0f);
        Point2 tex_g = Point2(0.0f, 1.0f);
        Point2 tex_h = Point2(1.0f, 1.0f);
        Point2 texCoords2[] = {tex_e, tex_f, tex_g, tex_h};
        quads[1] = Quad(vertices2, colors, texCoords2);
        position = Point3(0.0f, 0.0f, 0.0f);
    }

    /// OpenGL size of vertices of the Quads to be uploaded to OpenGL
    size_t size_of_vertices() {
        size_t size = 0;
        for (auto quad : quads) {
            size += quad.size_of_vertices();
        }
        return size;
    }
};

#endif //MEINEKRAFT_PRIMITIVES_H
