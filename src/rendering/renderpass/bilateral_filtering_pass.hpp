#ifndef BILATERAL_FILTERING_RENDERPASS_HPP
#define BILATERAL_FILTERING_RENDERPASS_HPP

#include "renderpass.hpp"

#include <stdint.h>

struct Shader;
struct GbufferRenderPass;

struct BilateralFilteringRenderPass: public RenderPass {
  /// Renderpass dependencies
  GbufferRenderPass* gbuffer_pass = nullptr;

  Shader* ping_shader = nullptr;
  Shader* pong_shader = nullptr;

  // Bilateral filtering shader pass
  Shader* bf_ping_shader = nullptr;
  Shader* bf_pong_shader = nullptr;
  uint32_t gl_bf_vao = 0;
  uint32_t gl_bf_ping_fbo = 0;
  uint32_t gl_bf_pong_fbo = 0;
  uint32_t gl_bf_ping_out_texture = 0;
  uint32_t gl_bf_ping_out_texture_unit = 0;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif // BILATERAL_FILTERING_RENDERPASS_HPP
