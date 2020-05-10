#ifndef LIGHTING_APPLICATION_RENDERPASS_HPP
#define LIGHTING_APPLICATION_RENDERPASS_HPP

#include "renderpass.hpp"

#include <stdint.h>

struct Shader;
struct GbufferRenderPass;
struct VoxelConeTracingRenderPass;

struct LightingApplicationRenderPass: public RenderPass {
  // RenderPass dependencies
  GbufferRenderPass* gbuffer_pass = nullptr;
  VoxelConeTracingRenderPass* voxel_cone_tracing_pass = nullptr;

  /// Lighting application pass related
  Shader* shader = nullptr;
  uint32_t gl_lighting_application_fbo;
  uint32_t gl_lighting_application_texture;
  uint32_t gl_lighting_application_texture_unit;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif // LIGHTING_APPLICATION_RENDERPASS_HPP
