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

struct RenderPass;
struct DownsampleRenderPass;
struct GbufferRenderPass;
struct DirectionalShadowRenderPass;
struct LightingApplicationRenderPass;
struct DirectLightingRenderPass;
struct VoxelizationRenderPass;
struct VoxelConeTracingRenderPass;

// GOAL WITH RENDERPASS REFACTOR:
// - Nothing about the render passes shall be exposed through the Renderer interface
// - The Renderer is the middleware between the game engine and the graphics API used
// -- Game engine: Yo, I want this to show on screen. Here are the assets (RenderComponent)
// -- Renderer: Sure, I will manage the rendering of these assets. Here is an ID for the component you gave me.
// -- Game engine: Thanks, I will keep track of this.
// - The Renderer is responsible to provide facilities for the RenderPasses such that they may focus on the
//  rendering algorithms instead of state management.
//
//  RenderPipeline: Consists of multiple or a single RenderingPipeline
//  - Manages a set of RenderPasses s.t their inputs and outputs are satisfied
//  - Manages the RenderPass ordering (and resources?)

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

 
  DownsampleRenderPass* downsample_pass = nullptr;
  GbufferRenderPass* gbuffer_pass = nullptr;
  DirectionalShadowRenderPass* shadow_pass = nullptr;
  LightingApplicationRenderPass* lighting_application_pass = nullptr;
  DirectLightingRenderPass* direct_lighting_pass = nullptr;
  VoxelizationRenderPass* voxelization_pass = nullptr;
  VoxelConeTracingRenderPass* voxel_cone_tracing_pass = nullptr;
  std::vector<RenderPass*> render_passes;

  glm::mat4 camera_transform; // TODO
  glm::mat4 projection_matrix; // TODO

  // VOXELIZATION PASS + VCT PASS shared state
  /// Voxelization pipeline related
  static const uint32_t NUM_CLIPMAPS = 4;
  // In order from smallest to largest in term of space occupied
  struct {
    AABB aabb[NUM_CLIPMAPS];
    int32_t size[NUM_CLIPMAPS] = {64, 64, 64, 32};
  } clipmaps;

  // Voxels
  uint32_t gl_voxel_radiance_textures[NUM_CLIPMAPS] = {};
  int32_t gl_voxel_radiance_image_units[NUM_CLIPMAPS] = {};
  int32_t gl_voxel_radiance_texture_units[NUM_CLIPMAPS] = {};

  uint32_t gl_voxel_opacity_textures[NUM_CLIPMAPS] = {};
  int32_t gl_voxel_opacity_image_units[NUM_CLIPMAPS] = {};
  int32_t gl_voxel_opacity_texture_units[NUM_CLIPMAPS] = {};

  // TODO: Docs
  glm::mat4 orthographic_projection(const AABB& aabb);

  // Render pass handling related
  /// Called when a rendering pass is started
  void pass_started(const std::string &msg);
  /// Called when a rendering pass is ended
  void pass_ended() const;

  // TODO: Document
  uint32_t get_next_free_texture_unit(bool peek = false);

  // TODO: Document
  uint32_t get_next_free_image_unit(bool peek = false);

  // TODO: Remove querying ...
  const static size_t MAX_RENDER_PASSES = 25;
  /// Render pass execution time query buffer
  uint32_t gl_execution_time_query_buffer = 0;
  uint64_t* gl_query_time_buffer_ptr = nullptr;
  std::array<uint32_t, MAX_RENDER_PASSES> gl_query_ids = {};

  Scene *scene = nullptr;
  std::vector<PointLight> pointlights;
// private:

  // TODO: Document
  void add_graphics_state(GraphicsBatch& batch, const RenderComponent& comp, Material material, ID entity_id);

  // TODO: Document
  void update_transforms();

  // TODO: Document
  void link_batch(GraphicsBatch& batch);

  /// View frustum culling shader
  ComputeShader* cull_shader = nullptr;

  uint32_t gl_pointlights_ssbo = 0;
  uint8_t* gl_pointlights_ssbo_ptr = nullptr;

  // Bilateral filtering shader pass
  Shader* bf_ping_shader = nullptr;
  Shader* bf_pong_shader = nullptr;
  uint32_t gl_bf_vao = 0;
  uint32_t gl_bf_ping_fbo = 0;
  uint32_t gl_bf_pong_fbo = 0;
  uint32_t gl_bf_ping_out_texture = 0;
  uint32_t gl_bf_ping_out_texture_unit = 0;

  // Bilateral upsampling shader pass
  Shader* bs_ping_shader = nullptr;
  uint32_t gl_bs_vao = 0;
  uint32_t gl_bs_ping_fbo = 0;
  uint32_t gl_bs_ping_out_texture = 0;
  uint32_t gl_bs_ping_out_texture_unit = 0;

  // Filtering related
  std::vector<float> kernel = {}; // Gaussian 1D separable kernel weights

  // Bilinear upsampling pass
  Shader* bilinear_upsampling_shader = nullptr;
  uint32_t gl_bilinear_upsampling_fbo = 0;
  uint32_t gl_bilinear_upsampling_texture = 0;
  uint32_t gl_bilinear_upsampling_texture_unit = 0;

  //    Environment map
  Texture environment_map;
  uint32_t gl_environment_map_texture_unit = 0;
};

#endif // MEINEKRAFT_RENDERER_HPP
