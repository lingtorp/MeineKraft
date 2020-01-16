#pragma once
#ifndef MEINEKRAFT_RENDERER_HPP
#define MEINEKRAFT_RENDERER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "texture.hpp"
#include "light.hpp"
#include "../rendering/primitives.hpp"

#include <glm/mat4x4.hpp>

struct Camera;
struct RenderComponent;
struct GraphicsBatch;
struct Shader;
struct ComputeShader;
struct Material;
struct Texture;
struct Scene;

struct Renderer {
  /// Create a renderer with a given window/screen size/resolution 
  Renderer(const Resolution& screen);
  ~Renderer();

  /// Post allocation initialization
  bool init();

  /// Main render function, renders all the graphics batches
  void render(const uint32_t delta);
  
  /// Adds the data of a RenderComponent to a internal batch
  void add_component(const RenderComponent comp, const ID entity_id);

  void remove_component(ID entity_id); // TODO: Implement

  /// Updates all the shaders projection matrices in order to support resizing of the window
  void update_projection_matrix(const float fov, const Resolution& screen);

  void load_environment_map(const std::vector<std::string>& faces);

  RenderState state;
  Resolution screen;
  std::vector<GraphicsBatch> graphics_batches;

	Scene *scene = nullptr;
  std::vector<PointLight> pointlights;
  DirectionalLight directional_light = DirectionalLight(Vec3f(0.0f, 0.5f, 0.5f), Vec3f(0.0f, -1.0f, -0.3f));
private:
  glm::mat4 projection_matrix; 

  void add_graphics_state(GraphicsBatch& batch, const RenderComponent& comp, Material material, ID entity_id);
  void update_transforms();
  void link_batch(GraphicsBatch& batch);

  /// View frustum culling shader
  ComputeShader* cull_shader = nullptr;

  /// Render pass execution time query buffer
  uint32_t gl_execution_time_query_buffer = 0;
  uint32_t *gl_query_ids = nullptr;

  /// Lighting application pass related
  Shader* lighting_application_shader = nullptr;
  uint32_t gl_lighting_application_fbo;
  uint32_t gl_lighting_application_texture;
  uint32_t gl_lighting_application_texture_unit;
  
  /// Geometry pass related
  uint32_t gl_depth_fbo = 0;
  uint32_t gl_depth_texture = 0;
  uint32_t gl_depth_texture_unit = 0;

  /// Directional shadow mapping related
  uint32_t gl_shadowmapping_fbo = 0;
  uint32_t gl_shadowmapping_texture = 0;
  uint32_t gl_shadowmapping_texture_unit = 0;
  Shader* shadowmapping_shader = nullptr;
  const uint32_t SHADOWMAP_W = 2 * 2048; // Shadowmap texture dimensions
  const uint32_t SHADOWMAP_H = SHADOWMAP_W;

  uint32_t gl_pointlights_ssbo = 0;
  uint8_t* gl_pointlights_ssbo_ptr = nullptr;

  /// Voxelization pipeline related
  static const uint32_t NUM_CLIPMAPS = 4;
  // In order from smallest to largest in term of space occupied
  struct {
    AABB aabb[NUM_CLIPMAPS];
    int32_t size[NUM_CLIPMAPS] = {64, 64, 64, 32};
  } clipmaps;

  Shader* voxelization_shader = nullptr;
  uint32_t gl_voxelization_fbo = 0;
  ComputeShader* voxelization_opacity_norm_shader = nullptr;

  // Rasterization based VCT pass related
  Shader* vct_shader = nullptr;
  uint32_t gl_vct_fbo = 0;
  uint32_t gl_vct_vao = 0;

  uint32_t gl_vct_diffuse_cones_ssbo = 0;
  uint8_t* gl_vct_diffuse_cones_ssbo_ptr = nullptr;

  // Bilateral filtering rasterization shader pass related
  Shader* bf_ping_shader = nullptr;
  Shader* bf_pong_shader = nullptr;
  uint32_t gl_bf_vao = 0;
  uint32_t gl_bf_ping_fbo = 0;
  uint32_t gl_bf_pong_fbo = 0;
  uint32_t gl_bf_ping_out_texture = 0;
  uint32_t gl_bf_ping_out_texture_unit = 0;

  // Filtering related
  std::vector<float> kernel = {}; // Gaussian 1D separable kernel weights

  // Voxels
  uint32_t gl_voxel_radiance_textures[NUM_CLIPMAPS] = {};
  int32_t gl_voxel_radiance_image_units[NUM_CLIPMAPS] = {};
  int32_t gl_voxel_radiance_texture_units[NUM_CLIPMAPS] = {};

  uint32_t gl_voxel_opacity_textures[NUM_CLIPMAPS] = {};
  int32_t gl_voxel_opacity_image_units[NUM_CLIPMAPS] = {};
  int32_t gl_voxel_opacity_texture_units[NUM_CLIPMAPS] = {};

  // Voxel visualization pass
  Shader* voxel_visualization_shader = nullptr;
  uint32_t gl_voxel_visualization_vao = 0;
  uint32_t gl_voxel_visualization_fbo = 0;
  uint32_t gl_voxel_visualization_texture = 0;
  uint32_t gl_voxel_visualization_texture_unit = 0;

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
  // Emissive map
  uint32_t gl_emissive_texture_unit = 0;
  uint32_t gl_emissive_texture = 0;
  // Shading model 
  uint32_t gl_shading_model_texture_unit = 0;
  uint32_t gl_shading_model_texture = 0;
  // Radiance textures
  uint32_t gl_indirect_radiance_texture_unit = 0;
  uint32_t gl_indirect_radiance_texture = 0;

  uint32_t gl_ambient_radiance_texture_unit = 0;
  uint32_t gl_ambient_radiance_texture = 0;

  uint32_t gl_specular_radiance_texture_unit = 0;
  uint32_t gl_specular_radiance_texture = 0;

  uint32_t gl_direct_radiance_texture_unit = 0;
  uint32_t gl_direct_radiance_texture = 0;

  //  Environment map
  Texture environment_map; 
  uint32_t gl_environment_map_texture_unit = 0;
};

#endif // MEINEKRAFT_RENDERER_HPP
