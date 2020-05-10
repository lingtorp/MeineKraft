#ifndef DOWNSAMPLE_RENDERPASS_HPP
#define DOWNSAMPLE_RENDERPASS_HPP

#include <stdint.h>

#include "renderpass.hpp"

struct Renderer;
struct Shader;
struct GbufferRenderPass;
struct DirectionalShadowRenderPass;

/// Downsampled global buffers
struct DownsampleRenderPass: public RenderPass {
  // Dependencies
  GbufferRenderPass* gbuffer_pass = nullptr;
  DirectionalShadowRenderPass* shadow_pass = nullptr;

  // Renderpass state
  Shader* shader = nullptr;
  uint32_t gl_vao = 0;
  uint32_t gl_fbo = 0;

  // FIXME: Not working due to depth map being broken?
  uint32_t gl_depth_texture = 0;
  uint32_t gl_depth_texture_unit = 0;

  uint32_t gl_position_texture = 0;
  uint32_t gl_position_texture_unit = 0;

  uint32_t gl_normal_texture = 0;
  uint32_t gl_normal_texture_unit = 0;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif // DOWNSAMPLE_RENDERPASS_HPP
