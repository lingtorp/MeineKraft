#pragma once
#ifndef MEINEKRAFT_RENDERER_HPP
#define MEINEKRAFT_RENDERER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "texture.hpp"
#include "light.hpp"
#include "../rendering/primitives.hpp"
#include "renderpass.hpp"

#include <glm/mat4x4.hpp>

struct Camera;
struct RenderComponent;
struct GraphicsBatch;
struct Shader;
struct ComputeShader;
struct Material;

struct Renderer {
  /// Create a renderer with a given window/screen size/resolution
  Renderer(const Resolution& screen);
  ~Renderer();

  /// Main render function, renders all the graphics batches
  void render(const uint32_t delta);
  
  /// Adds the data of a RenderComponent to a internal batch
  void add_component(const RenderComponent comp, const ID entity_id);

  void remove_component(ID entity_id); // TODO: Implement

  /// Updates all the shaders projection matrices in order to support resizing of the window
  void update_projection_matrix(const float fov, const Resolution& screen);

  /// Returns the next unused texture unit
  static uint32_t get_next_free_texture_unit();

  void load_environment_map(const std::vector<std::string>& faces);

  // Rudimentary rendering pipeline for now
  std::vector<RenderPass> render_passes;

  Camera* camera = nullptr;
  RenderState state;
  glm::mat4 projection_matrix; 
  Resolution screen;
  std::vector<GraphicsBatch> graphics_batches;
  std::vector<PointLight> pointlights; 

  // Shadow mapping
  DirectionalLight directional_light = DirectionalLight(Vec3f(-130.0, 2000.0, 0.0), Vec3f(0.0, -0.9, 0.523));
private:
  void add_graphics_state(GraphicsBatch& batch, const RenderComponent& comp, Material material, ID entity_id);
  void update_transforms();
  void link_batch(GraphicsBatch& batch);

  /// View frustum culling shader
  ComputeShader* cull_shader = nullptr;
  
  /// Geometry pass related
  uint32_t gl_depth_fbo = 0;
  uint32_t gl_depth_texture = 0;
  uint32_t gl_depth_texture_unit = 0;

  /// Directional shadow mapping related
  uint32_t gl_shadowmapping_fbo = 0;
  uint32_t gl_shadowmapping_texture = 0;
  uint32_t gl_shadowmapping_texture_unit = 0;
  Shader* shadowmapping_shader = nullptr;
  const uint32_t SHADOWMAP_W = 4 * 1024; // Shadowmap texture dimensions
  const uint32_t SHADOWMAP_H = 4 * 1024;

  /// Lightning pass related
  Shader* lightning_shader = nullptr;
  // Used since default fbo is not to be trusted
  uint32_t gl_lightning_texture = 0;
  uint32_t gl_lightning_texture_unit = 0;
  uint32_t gl_lightning_fbo = 0;
  uint32_t gl_lightning_vao = 0;

  uint32_t gl_pointlights_ssbo = 0;
  uint8_t* gl_pointlights_ssbo_ptr = nullptr;

  /// Global buffers
  // Geometric normals
  uint32_t gl_geometric_normal_texture = 0;
  uint32_t gl_geometric_normal_texture_unit = 0;
  // Tangent space normals
  uint32_t gl_tangent_normal_texture = 0;
  uint32_t gl_tangent_normal_texture_unit = 0;
  // Tangent map
  uint32_t gl_tangent_texture = 0;
  uint32_t gl_tangent_texture_unit = 0;
  // Positions
  uint32_t gl_position_texture = 0;
  uint32_t gl_position_texture_unit = 0;
  // Diffuse
  uint32_t gl_diffuse_texture = 0;
  uint32_t gl_diffuse_texture_unit = 0;
  // PBR Parameters
  uint32_t gl_pbr_parameters_texture = 0;
  uint32_t gl_pbr_parameters_texture_unit = 0;
  // Ambient occlusion map
  uint32_t gl_ambient_occlusion_texture = 0;
  uint32_t gl_ambient_occlusion_texture_unit = 0;
  // Emissive map
  uint32_t gl_emissive_texture_unit = 0;
  uint32_t gl_emissive_texture = 0;
  // Shading model 
  uint32_t gl_shading_model_texture_unit = 0;
  uint32_t gl_shading_model_texture = 0;

  // Environment map
  Texture environment_map; 
  uint32_t gl_environment_map_texture_unit = 0;
};

#endif // MEINEKRAFT_RENDERER_HPP
