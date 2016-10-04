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
    uint64_t graphic_batches;
};

enum ShaderType {
    STANDARD_SHADER, SKYBOX_SHADER
};

class Renderer {
public:
    Renderer(Renderer &render) = delete;
    ~Renderer(); // Render does not dealloc properly atm

    /// Singleton instance of core Render, use with caution.
    static Renderer &instance() {
        static Renderer instance;
        return instance;
    }

    /// Main render function, renders all the graphics batches and so on
    void render();

    /// Adds the RenderComponent to a internal batch with the same Entity.hash_id
    uint64_t add_to_batch(RenderComponent component);

    /// Removes the RenderComponent from a internal batch with the same Entity.hash_id
    void remove_from_batch(RenderComponent component);

    /// Updates the RenderComponent to a internal batch with the same Entity.hash_id and RenderComponent.id
    void update_render_component(RenderComponent component);

    Mat4<float> FPSViewRH(Vec3<float> eye, float pitch, float yaw);

    /// Updates all the shaders projection matrices in order to support resizing of the window
    void update_projection_matrix(float fov);

    std::shared_ptr<Camera> camera;
    RenderState state;
    SDL_Window *window;
private:
    Renderer();
    uint64_t render_components_id;
    double DRAW_DISTANCE;

    Mat4<float> projection_matrix;

    std::unordered_map<Texture, uint64_t, std::hash<int>> textures;
    std::vector<GraphicsBatch> graphics_batches;
    std::unordered_map<ShaderType, Shader, std::hash<int>> shaders;

    bool point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes);
    std::array<Plane<float>, 6> extract_planes(Mat4<float> matrix);
};

#endif //MEINEKRAFT_RENDER_H
