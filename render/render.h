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

#include <glm/mat4x4.hpp>

struct Camera;
class RenderComponent;
class GraphicsBatch;
struct Shader;
class RenderPass;

class Renderer {
public:
  Renderer(Renderer& render) = delete;
  ~Renderer();

  /// Singleton instance of core Render, use with caution.
  static Renderer& instance() {
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
  static uint32_t get_next_free_texture_unit();

  void load_environment_map(std::string directory, std::string file);

  Camera* camera;
  RenderState state;
  glm::mat4 projection_matrix; 
  float screen_width;
  float screen_height;

private:
  Renderer();

  std::vector<GraphicsBatch> graphics_batches;

  /// Setups the VAO and uniforms up between the batch and OpenGL
  void link_batch(GraphicsBatch& batch);
  
  /// Geometry pass related
  Shader* depth_shader;
  uint32_t gl_depth_fbo;
  uint32_t gl_depth_texture;
  uint32_t gl_depth_texture_unit;

  /// Lightning pass related
  Shader* lightning_shader;
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
  // Diffuse
  uint32_t gl_diffuse_texture;
  uint32_t gl_diffuse_texture_unit;
  // PBR Parameters
  uint32_t gl_pbr_parameters_texture;
  uint32_t gl_pbr_parameters_texture_unit;
  // Ambient occlusion map
  uint32_t gl_ambient_occlusion_texture;
  uint32_t gl_ambient_occlusion_texture_unit;
  // Emissive map
  uint32_t gl_emissive_texture_unit;
  uint32_t gl_emissive_texture;
  // Shading model 
  uint32_t gl_shading_model_texture_unit;
  uint32_t gl_shading_model_texture;

  // Environment map
  uint32_t gl_hdr_environment_texture; 

  std::vector<RenderPass> render_passes;
};

#endif // MEINEKRAFT_RENDER_H
