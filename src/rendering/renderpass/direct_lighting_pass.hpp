#ifndef  DIRECT_LIGHTING_RENDERPASS_HPP
#define  DIRECT_LIGHTING_RENDERPASS_HPP

#include "renderpass.hpp"

#include <stdint.h>

struct Shader;
struct DirectionalShadowRenderPass;
struct GbufferRenderPass;

struct DirectLightingRenderPass: public RenderPass {
  /// RenderPass dependencies
  DirectionalShadowRenderPass* shadow_pass = nullptr;
  GbufferRenderPass* gbuffer_pass = nullptr;

  /// Direct lighting shadow pass related
  Shader* shader = nullptr;
  uint32_t gl_direct_lighting_fbo = 0;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif // DIRECT_LIGHTING_RENDERPASS_HPP
