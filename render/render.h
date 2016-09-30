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
    static Mat4<GLfloat> FPSViewRH(Vec3<> eye, float pitch, float yaw);
    void update_projection_matrix();

    std::shared_ptr<Camera> camera;
private:
    SDL_Window *window;
    Cube skybox;
    double DRAW_DISTANCE;
    std::unordered_map<Texture, GLuint, std::hash<int>> textures;
    Mat4<GLfloat> projection_matrix;

    GLuint gl_VAO;
    GLuint gl_modelsBO;
    GLuint gl_camera_view;
    GLuint gl_shader_program;

    GLuint gl_skybox_shader;
    GLint gl_skybox_camera;
    GLint gl_skybox_model;
    GLuint gl_skybox_VAO;

    bool point_inside_frustrum(Vec3<GLfloat> point, std::array<Plane<GLfloat>, 6> planes);
    std::array<Plane<GLfloat>, 6> extract_planes(Mat4<GLfloat> matrix);
};

#endif //MEINEKRAFT_RENDER_H
