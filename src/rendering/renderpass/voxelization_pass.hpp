#ifndef VOXELIZATION_RENDERPASS_HPP
#define VOXELIZATION_RENDERPASS_HPP

#include "renderpass.hpp"

#include <stdint.h>

struct Shader;
struct GbufferRenderPass;
struct DirectionalShadowRenderPass;

struct VoxelizationRenderPass: public RenderPass {
  /// RenderPass dependencies
  GbufferRenderPass* gbuffer_pass = nullptr;
  DirectionalShadowRenderPass* shadow_pass = nullptr;

  Shader* shader = nullptr;
  uint32_t gl_voxelization_fbo = 0;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif // VOXELIZATION_RENDERPASS_HPP
