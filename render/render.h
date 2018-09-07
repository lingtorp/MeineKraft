#ifndef MEINEKRAFT_RENDER_H
#define MEINEKRAFT_RENDER_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <memory>

#include "texture.h"
#include "light.h"
#include "transform.h"
#include "shader.h"

#ifdef _WIN32
#include <SDL_video.h>
#else
#include <sdl2/SDL_video.h>
#endif 

struct World;
class Camera;
class RenderComponent;
class GraphicsBatch;
class Shader;
struct MeshManager;
struct RenderPass;

class Renderer {
public:
  Renderer(Renderer &render) = delete;
  ~Renderer();

  /// Singleton instance of core Render, use with caution.
  static Renderer &instance() {
    static Renderer instance;
    return instance;
  }

  /// Main render function, renders all the graphics batches
  void render(uint32_t delta);
  
  /// Adds the RenderComponent to a internal batch
  uint64_t add_to_batch(RenderComponent* comp);

  /// Updates all the shaders projection matrices in order to support resizing of the window
  void update_projection_matrix(const float fov);

  /// Returns the next unused texture unit
  uint32_t get_next_free_texture_unit();
  int32_t next_texture_unit;

  std::vector<RenderPass> passes;

  Camera* camera;
  RenderState state;
  Mat4<float> projection_matrix; // TODO: Generate on demand
  float screen_width;
  float screen_height;

private:
  Renderer();
  
  /// Geometry pass related
  Shader* depth_shader;
  uint32_t gl_depth_fbo;
  uint32_t gl_depth_texture;
  uint32_t gl_depth_texture_unit;
  
  /// Blur pass related
  Shader* blur_shader;
  uint32_t gl_blur_fbo;
  uint32_t gl_blur_texture;
  uint32_t gl_blur_texture_unit;
  uint32_t gl_blur_vao;

  /// Lightning pass related
  Shader* lightning_shader;
  // Point light
  uint32_t gl_pointlight_models_buffer_object;
  // Used since default fbo is not to be trusted
  uint32_t gl_lightning_texture;
  uint32_t gl_lightning_fbo;
  uint32_t gl_lightning_texture_unit;
  uint32_t gl_lightning_vao;

  /// Global buffers
  // Normals
  uint32_t gl_normal_texture;
  uint32_t gl_normal_texture_unit;
  // Positions
  uint32_t gl_position_texture;
  uint32_t gl_position_texture_unit;
  // Albedo
  uint32_t gl_albedo_texture;
  uint32_t gl_albedo_texture_unit;

  std::vector<GraphicsBatch> graphics_batches;

  bool point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes);

  std::array<Plane<float>, 6> extract_planes(Mat4<float> matrix);

  /// Setups the VAO and uniforms up between the batch and OpenGL
  void link_batch(GraphicsBatch &batch);
};

#endif // MEINEKRAFT_RENDER_H
