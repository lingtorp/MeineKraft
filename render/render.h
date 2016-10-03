#ifndef MEINEKRAFT_RENDER_H
#define MEINEKRAFT_RENDER_H

#include <cstdint>
#include <SDL2/SDL_video.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "primitives.h"

class World;
class Camera;
class RenderComponent;
class GraphicsBatch;
class Shader;
struct Cube;

enum Texture: uint64_t;

struct RenderState {
    uint64_t entities;
};

enum ShaderType {
    STANDARD_SHADER, SKYBOX_SHADER
};

class Render {
public:
    Render(Render &render) = delete;
    ~Render(); // Render does not dealloc properly atm

    /// Singleton instance of core Render, use with caution.
    static Render &instance() {
        static Render instance;
        return instance;
    }

    void render_world(const World *world);

    void add_to_batch(RenderComponent component);
    void remove_from_batch(RenderComponent component);
    std::vector<GraphicsBatch> graphics_batches;
    std::unordered_map<ShaderType, Shader, std::hash<int>> shaders;

    // std::shared_ptr<GraphicsState> graphics_state(uint64_t hash_id, std::string obj_file);

    Mat4<float> FPSViewRH(Vec3<float> eye, float pitch, float yaw);
    void update_projection_matrix(float fov);

    std::shared_ptr<Camera> camera;
    RenderState state;

    SDL_Window *window;
private:
    Render();

    Cube skybox;
    double DRAW_DISTANCE;
    std::unordered_map<Texture, uint64_t, std::hash<int>> textures;
    Mat4<float> projection_matrix;

    uint64_t gl_VAO;
    uint64_t gl_modelsBO;
    uint64_t gl_camera_view;

    uint64_t gl_skybox_VAO;
    uint64_t gl_skybox_model;
    uint64_t gl_skybox_camera;

    bool point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes);
    std::array<Plane<float>, 6> extract_planes(Mat4<float> matrix);
};

#endif //MEINEKRAFT_RENDER_H
