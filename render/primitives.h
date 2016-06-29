#ifndef MEINEKRAFT_PRIMITIVES_H
#define MEINEKRAFT_PRIMITIVES_H

#include <SDL_opengl.h>
#include <cstdlib>
#include "../math/vector.h"

// Colors
struct Color4 { GLfloat r, g, b, a; };

static inline Color4 new_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    Color4 color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
}

static inline Color4 new_color_clear()  { return new_color(0.0f, 0.0f, 0.0f, 0.0f); }
static inline Color4 new_color_red()    { return new_color(1.0f, 0.0f, 0.0f, 1.0f); }
static inline Color4 new_color_blue()   { return new_color(0.0f, 1.0f, 0.0f, 1.0f); }
static inline Color4 new_color_green()  { return new_color(0.0f, 0.0f, 1.0f, 1.0f); }
static inline Color4 new_color_yellow() { return new_color(1.0f, 1.0f, 0.0f, 1.0f); }

struct Vertex {
    Point3 position;
    Color4 color;
    Point2 texCoord;
};

struct Quad { Vertex vertices[4]; };

typedef enum { SKYBOX, DIRT, GRASS, AIR } Texture;

struct Cube {
    Quad quads[2];
    Point3 position; // Center point of the cube
    GLfloat scale;
    GLfloat theta_x, theta_y, theta_z; // Rotation in object space
    Texture texture;
};

static Vertex new_vertex(Point3 point, Color4 color, Point2 texCoord) {
    Vertex vertex;
    vertex.color = color;
    vertex.position = point;
    vertex.texCoord = texCoord;
    return vertex;
}

// Input: 4 points, 4 colors
//  d ----- c   <-- Point & color pairs
//  |       |
//  a ----- b   0 = a, 1 = b, 2 = c, 3 = d
static Quad new_quad(Point3 *points, Color4 *colors, Point2 *texCoords) {
    Quad quad;
    Point3 a = points[0];
    Point3 b = points[1];
    Point3 c = points[2];
    Point3 d = points[3];
    quad.vertices[0] = new_vertex(a, colors[0], texCoords[0]);
    quad.vertices[1] = new_vertex(b, colors[1], texCoords[1]);
    quad.vertices[2] = new_vertex(c, colors[2], texCoords[2]);
    quad.vertices[3] = new_vertex(d, colors[3], texCoords[3]);
    return quad;
}

static Cube *new_cube(Color4 *colors) {
    Cube *cube = (Cube *) calloc(1, sizeof(Cube));
    cube->scale = 1.0f;
    Point3 a = new_point3(-0.5f, -0.5f, 0.5f);
    Point3 b = new_point3(0.5f, -0.5f, 0.5f);
    Point3 c = new_point3(0.5f, 0.5f, 0.5f);
    Point3 d = new_point3(-0.5f, 0.5f, 0.5f);
    Point3 vertices1[] = {a, b, c, d};
    Point2 tex_a = new_point2(0.0f, 0.0f);
    Point2 tex_b = new_point2(1.0f, 0.0f);
    Point2 tex_c = new_point2(1.0f, 1.0f);
    Point2 tex_d = new_point2(0.0f, 1.0f);
    Point2 texCoords1[] = {tex_a, tex_b, tex_c, tex_d};
    Quad quad1 = new_quad(vertices1, colors, texCoords1);
    cube->quads[0] = quad1;

    Point3 e = new_point3(-0.5f, -0.5f, -0.5f);
    Point3 f = new_point3(0.5f, -0.5f, -0.5f);
    Point3 g = new_point3(0.5f, 0.5f, -0.5f);
    Point3 h = new_point3(-0.5f, 0.5f, -0.5f);
    Point3 vertices2[] = {e, f, g, h};
    Point2 tex_e = new_point2(1.0f, 0.0f);
    Point2 tex_f = new_point2(0.0f, 0.0f);
    Point2 tex_g = new_point2(0.0f, 1.0f);
    Point2 tex_h = new_point2(1.0f, 1.0f);
    Point2 texCoords2[] = {tex_e, tex_f, tex_g, tex_h};
    Quad quad2 = new_quad(vertices2, colors, texCoords2);
    cube->quads[1] = quad2;
    cube->position = new_point3(0.0f, 0.0f, 0.0f);
    return cube;
}

#endif //MEINEKRAFT_PRIMITIVES_H
