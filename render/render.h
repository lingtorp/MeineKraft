#ifndef MEINEKRAFT_RENDER_H
#define MEINEKRAFT_RENDER_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <math.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <string>
#include <vector>
#include <cstdio>

#include "../math/vector.h"
#include "primitives.h"
#include "../world.h"

struct Camera {
    Vec3 direction;
    Vec3 position;
    Vec3 up;
    GLfloat pitch;
    GLfloat yaw;
    float movement_speed;
};

class Render {
public:
    GLfloat *FPSViewRH(Vec3 eye, float pitch, float yaw);
    void render_world(World *world);
    Render(SDL_Window *window);
    ~Render();

    const GLchar *load_shader_source(std::string filename);

    GLfloat *quad_to_floats(Quad *quad);
    GLfloat *transformation_matrix_x(float theta);
    GLfloat *transformation_matrix_y(float theta);
    GLfloat *transformation_matrix_z(float theta);
    GLfloat *scaling_matrix(float scale);
    GLfloat *translation_matrix(float x, float y, float z);

    Vec3 camera_move_forward(const Camera *cam);
    Vec3 camera_move_backward(const Camera *cam);
    Vec3 camera_move_right(const Camera *cam);
    Vec3 camera_move_left(const Camera *cam);
    Vec3 camera_direction(const Camera *cam);

    SDL_Window *window;
    Camera *camera;
    GLuint *textures;
    GLuint VBO;
    GLuint VAO;
    GLuint camera_view;
    GLuint transform_x;
    GLuint transform_y;
    GLuint transform_z;
    GLuint transform_translation;
    GLuint transform_scaling;
    Cube *skybox;
    GLuint shader_program;
};

#endif //MEINEKRAFT_RENDER_H
