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
struct ViewFrustumCullingRenderPass;
struct BilinearUpsamplingRenderPass;
struct BilateralFilteringRenderPass;
struct BilateralUpsamplingRenderPass;

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

  Scene *scene = nullptr;
  RenderState state;
  Resolution screen;
  std::vector<GraphicsBatch> graphics_batches;
  std::vector<PointLight> pointlights; // FIXME: Unused for now

  DownsampleRenderPass* downsample_pass = nullptr;
  GbufferRenderPass* gbuffer_pass = nullptr;
  DirectionalShadowRenderPass* shadow_pass = nullptr;
  LightingApplicationRenderPass* lighting_application_pass = nullptr;
  DirectLightingRenderPass* direct_lighting_pass = nullptr;
  VoxelizationRenderPass* voxelization_pass = nullptr;
  VoxelConeTracingRenderPass* voxel_cone_tracing_pass = nullptr;
  ViewFrustumCullingRenderPass* view_frustum_culling_pass = nullptr;
  BilinearUpsamplingRenderPass* bilinear_upsampling_pass = nullptr;
  BilateralFilteringRenderPass* bilateral_filtering_pass = nullptr;
  BilateralUpsamplingRenderPass* bilateral_upsampling_pass = nullptr;
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

  /// Called when a rendering pass is started
  void pass_started(const std::string &msg);
  /// Called when a rendering pass is ended
  void pass_ended() const;

  // TODO: Document
  uint32_t get_next_free_texture_unit(bool peek = false);

  // TODO: Document
  uint32_t get_next_free_image_unit(bool peek = false);

  // TODO: Document
  void add_graphics_state(GraphicsBatch& batch, const RenderComponent& comp, Material material, ID entity_id);

  // TODO: Document
  void update_transforms();

  // TODO: Document
  void link_batch(GraphicsBatch& batch);
};

#endif // MEINEKRAFT_RENDERER_HPP
