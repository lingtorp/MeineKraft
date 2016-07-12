#ifndef MEINEKRAFT_RENDER_H
#define MEINEKRAFT_RENDER_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <cmath>
#include <SDL2/SDL_image.h>
#include <cassert>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

#include "../math/vector.h"
#include "primitives.h"
#include "../world.h"
#include "camera.h"

class Render {
public:
    void render_world(World *world);
    Render(SDL_Window *window);
    ~Render();

    const std::string load_shader_source(std::string filename);

    static Mat4<GLfloat> FPSViewRH(Vec3 eye, float pitch, float yaw);
    Mat4<GLfloat> transformation_matrix_x(float theta);
    Mat4<GLfloat> transformation_matrix_y(float theta);
    Mat4<GLfloat> transformation_matrix_z(float theta);
    Mat4<GLfloat> scaling_matrix(float scale);
    Mat4<GLfloat> translation_matrix(float x, float y, float z);

    std::shared_ptr<Camera> camera;

private:
    SDL_Window *window;
    Cube skybox;
    GLuint *textures;
    GLuint VBO;
    GLuint VAO;
    GLuint camera_view;
    GLuint transform_x;
    GLuint transform_y;
    GLuint transform_z;
    GLuint transform_translation;
    GLuint transform_scaling;
    GLuint shader_program;
};

#endif //MEINEKRAFT_RENDER_H
