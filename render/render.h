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
#include <unordered_map>

#include "../math/vector.h"
#include "primitives.h"
#include "../world/world.h"
#include "camera.h"

class Render {
public:
    void render_world(const World *world);
    Render(SDL_Window *window);
    ~Render();

    const std::string load_shader_source(std::string filename);

    static Mat4<GLfloat> FPSViewRH(Vec3 eye, float pitch, float yaw);
    Mat4<GLfloat> transformation_matrix_x(float theta);
    Mat4<GLfloat> transformation_matrix_y(float theta);
    Mat4<GLfloat> transformation_matrix_z(float theta);

    std::shared_ptr<Camera> camera;

private:
    SDL_Window *window;
    Cube skybox;
    std::unordered_map<Texture, GLuint, std::hash<int>> textures;
    GLuint gl_VBO;
    GLuint gl_VAO;
    GLuint gl_camera_view;
    GLuint gl_model;
    GLuint gl_shader_program;
};

#endif //MEINEKRAFT_RENDER_H
