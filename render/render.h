#ifndef MEINEKRAFT_RENDER_H
#define MEINEKRAFT_RENDER_H

#include <cstdint>
#include <SDL2/SDL_video.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include "primitives.h"
#include "texture.h"
#include "light.h"
#include "transform.h"

class World;
class Camera;
class RenderComponent;
class GraphicsBatch;
class Shader;
class FileMonitor;
class MeshManager;
class TextureManager;

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
    void render(uint32_t delta);

    /// Request a loading of a mesh, return mesh_id
    uint64_t load_mesh(GraphicsState *state, std::string filepath, std::string directory);

    /// Request a loading of a standard primitive mesh
    uint64_t load_mesh_primitive(MeshPrimitive primitive);

    /// Adds the RenderComponent to a internal batch
    void add_to_batch(RenderComponent *component, uint64_t mesh_id);

    /// Removes the RenderComponent from a internal batch with the same Entity.hash_id
    void remove_from_batch(RenderComponent *component);

    /// Updates all the shaders projection matrices in order to support resizing of the window
    void update_projection_matrix(float fov);

    ///
    Texture setup_texture(RenderComponent *component, Texture texture); // FIXME

    std::shared_ptr<Camera> camera;
    RenderState state;
    SDL_Window *window;
private:
    Renderer();
    double DRAW_DISTANCE;

    Mat4<float> projection_matrix;

    uint16_t MAX_NUM_LIGHTS = 100;
    uint32_t gl_light_uniform_buffer;
    std::vector<Light> lights;

    std::vector<Transform> transformations;

    std::vector<GraphicsBatch> graphics_batches;

    MeshManager *mesh_manager;

    TextureManager *texture_manager;

    std::unique_ptr<FileMonitor> shader_file_monitor;

    bool point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes);
    std::array<Plane<float>, 6> extract_planes(Mat4<float> matrix);

    /// Setups the VAO and uniforms up between the batch and OpenGL
    void link_batch(GraphicsBatch &batch, const GraphicsState &state);

    /// Creates a camera view matrix based on the euler angles (x, y) and position of the eye
    Mat4<float> FPSViewRH(Vec3<float> eye, float pitch, float yaw);
};

#endif //MEINEKRAFT_RENDER_H
