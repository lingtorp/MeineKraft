#pragma once
#ifndef MEINEKRAFT_RENDER_H
#define MEINEKRAFT_RENDER_H

#include <cstdint>
#include <string>
#include <vector>

#include "texture.hpp"
#include "light.hpp"
#include "../render/primitives.hpp"

#include <glm/mat4x4.hpp>

struct Camera;
struct RenderComponent;
struct GraphicsBatch;
struct Shader;
struct ComputeShader;
struct RenderPass;
struct Material;

struct Renderer {
public:
  Renderer(Renderer& render) = delete;
  ~Renderer();

  /// Singleton instance of core Render, use with caution.
  static Renderer& instance() {
    static Renderer instance;
    return instance;
  }

  /// Main render function, renders all the graphics batches
  void render(const uint32_t delta);
  
  /// Adds the data of a RenderComponent to a internal batch
  void add_component(const RenderComponent comp, const ID entity_id);

  void remove_component(ID entity_id);

  /// Updates all the shaders projection matrices in order to support resizing of the window
  void update_projection_matrix(const float fov);

  /// Returns the next unused texture unit
  static uint32_t get_next_free_texture_unit();

  void load_environment_map(const std::vector<std::string>& faces);

  Camera* camera = nullptr;
  RenderState state;
  glm::mat4 projection_matrix; 
  float screen_width;
  float screen_height;
  std::vector<GraphicsBatch> graphics_batches;
  std::vector<PointLight> pointlights;

private:
  Renderer();
  void add_graphics_state(GraphicsBatch& batch, const RenderComponent& comp, Material material, ID entity_id);
  void update_transforms();
  void link_batch(GraphicsBatch& batch);

  /// View frustum culling shader
  ComputeShader* cull_shader = nullptr;
  
  /// Geometry pass related
  uint32_t gl_depth_fbo;
  uint32_t gl_depth_texture;
  uint32_t gl_depth_texture_unit;

  /// Directional shadow mapping related
  uint32_t gl_shadowmapping_fbo = 0;
  uint32_t gl_shadowmapping_texture = 0;
  uint32_t gl_shadowmapping_texture_unit = 0;
  Shader* shadowmapping_shader = nullptr;
  const uint32_t SHADOWMAP_W = 1024; // Shadowmap texture dimensions
  const uint32_t SHADOWMAP_H = 1024;

  /// Lightning pass related
  Shader* lightning_shader;
  // Used since default fbo is not to be trusted
  uint32_t gl_lightning_texture;
  uint32_t gl_lightning_fbo;
  uint32_t gl_lightning_texture_unit;
  uint32_t gl_lightning_vao;

  uint32_t gl_pointlights_ssbo;
  uint8_t* gl_pointlights_ssbo_ptr = nullptr;

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
  Texture environment_map; 
  uint32_t gl_environment_map_texture_unit;
};

#endif // MEINEKRAFT_RENDER_H
