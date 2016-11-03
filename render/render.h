#ifndef MEINEKRAFT_RENDER_H
#define MEINEKRAFT_RENDER_H

#include <cstdint>
#include <SDL2/SDL_video.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "primitives.h"
#include "texture.h"

class World;
class Camera;
class RenderComponent;
class GraphicsBatch;
class Shader;
class FileMonitor;

struct Cube;

class Renderer {
public:
    Renderer(Renderer &render) = delete;
    ~Renderer(); // FIXME: Render does not dealloc properly atm

    /// Singleton instance of core Render, use with caution.
    static Renderer &instance() {
        static Renderer instance;
        return instance;
    }

    /// Main render function, renders all the graphics batches and so on
    void render();

    /// Loads a mesh from a file
    Mesh load_mesh_from_file(std::string filepath, std::string directory_filepath);

    /// Adds the RenderComponent to a internal batch with the same Entity.hash_id
    void add_to_batch(RenderComponent *component, Mesh mesh);

    /// Removes the RenderComponent from a internal batch with the same Entity.hash_id
    void remove_from_batch(RenderComponent *component);

    /// Creates a camera view matrix based on the euler angles (x, y) and position of the eye
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

    // std::unordered_map<Texture, uint64_t, std::hash<int>> textures;
    std::unordered_map<ShaderType, Shader, std::hash<int>> shaders;

    std::vector<GraphicsBatch> graphics_batches;

    bool point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes);
    std::array<Plane<float>, 6> extract_planes(Mat4<float> matrix);

    std::unique_ptr<FileMonitor> shader_file_monitor;

    /// Setups the VAO and uniforms up between the batch and OpenGL
    void link_batch(GraphicsBatch &batch);
};

#endif //MEINEKRAFT_RENDER_H
