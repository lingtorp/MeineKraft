#pragma once
#ifndef MEINEKRAFT_RENDERER_HPP
#define MEINEKRAFT_RENDERER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "texture.hpp"
#include "light.hpp"
#include "../rendering/primitives.hpp"

#include <glm/mat4x4.hpp>

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

  /// Post allocation initialization called at engine startup
  bool init();

  /// Renders one frame function using the Renderstate in 'state'
  void render(const uint32_t delta);
  
  /// Adds the data of a RenderComponent to a internal batch
  void add_component(const RenderComponent comp, const ID entity_id);

  /// Removes the RenderComponent associated with the EID if there exists one
  void remove_component(ID entity_id);

  // TODO: Document ...
  void load_environment_map(const std::array<std::string, 6>& faces);

  /// Returns the default framebuffer color in callee-owned ptr
  Vec3f* take_screenshot(const uint32_t gl_fbo = 0) const;

  RenderState state;
  Resolution screen;
  std::vector<GraphicsBatch> graphics_batches;

	Scene *scene = nullptr;
  std::vector<PointLight> pointlights;
private:
  // Render pass handling related
  /// Called when a rendering pass is started
  void pass_started(const std::string &msg);
  /// Called when a rendering pass is ended
  void pass_ended();

  const static size_t MAX_RENDER_PASSES = 25;
  /// Render pass execution time query buffer
  uint32_t gl_execution_time_query_buffer = 0;
  uint64_t* gl_query_time_buffer_ptr = nullptr;
  std::array<uint32_t, MAX_RENDER_PASSES> gl_query_ids = {};

  // TODO: Document
  void add_graphics_state(GraphicsBatch& batch, const RenderComponent& comp, Material material, ID entity_id);

  // TODO: Document
  void update_transforms();

  // TODO: Document
  void link_batch(GraphicsBatch& batch);

  /// View frustum culling shader
  ComputeShader* cull_shader = nullptr;

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
  Shader* shadowmapping_shader = nullptr;
  uint32_t gl_shadowmapping_fbo = 0;
  uint32_t gl_shadowmapping_texture = 0;
  uint32_t gl_shadowmapping_texture_unit = 0;

  /// Direct lighting shadow pass related
  Shader* direct_lighting_shader = nullptr;
  uint32_t gl_direct_lighting_fbo = 0;

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

  // Bilinear upsampling pass
  Shader* bilinear_upsampling_shader = nullptr;
  uint32_t gl_bilinear_upsampling_fbo = 0;
  uint32_t gl_bilinear_upsampling_texture = 0;
  uint32_t gl_bilinear_upsampling_texture_unit = 0;

  /// Global buffers
  // Geometric normals
  uint32_t gl_geometric_normal_texture = 0;
  uint32_t gl_geometric_normal_texture_unit = 0;
  //  Tangent space normals
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
  // Indirect radiance from GI
  uint32_t gl_indirect_radiance_texture_unit = 0;
  uint32_t gl_indirect_radiance_texture = 0;
  // Ambient radiance from GI
  uint32_t gl_ambient_radiance_texture_unit = 0;
  uint32_t gl_ambient_radiance_texture = 0;
  // Specular radiance from GI
  uint32_t gl_specular_radiance_texture_unit = 0;
  uint32_t gl_specular_radiance_texture = 0;
  // Direct radiance
  uint32_t gl_direct_radiance_texture_unit = 0;
  uint32_t gl_direct_radiance_texture = 0;

  /// Downsampled global buffers
  struct {
    Shader* shader = nullptr;
    uint32_t gl_vao = 0;
    uint32_t gl_fbo = 0;

    // FIXME: Not working due to depth map being broken
    uint32_t gl_depth_texture = 0;
    uint32_t gl_depth_texture_unit = 0;

    uint32_t gl_position_texture = 0;
    uint32_t gl_position_texture_unit = 0;

    uint32_t gl_normal_texture = 0;
    uint32_t gl_normal_texture_unit = 0;
  } gbuffer_downsampled;
 
  //    Environment map
  Texture environment_map;
  uint32_t gl_environment_map_texture_unit = 0;
};

#endif // MEINEKRAFT_RENDERER_HPP
