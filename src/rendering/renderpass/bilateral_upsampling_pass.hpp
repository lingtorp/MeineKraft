#ifndef BILATERAL_UPSAMPLING_RENDERPASS
#define BILATERAL_UPSAMPLING_RENDERPASS

#include "renderpass.hpp"

#include <stdint.h>

struct DownsampleRenderPass;
struct VoxelConeTracingRenderPass;
struct GbufferRenderPass;
struct Shader;

struct BilateralUpsamplingRenderPass: public RenderPass {
  /// Renderpass dependencies
  GbufferRenderPass* gbuffer_pass = nullptr;
  DownsampleRenderPass* downsample_pass = nullptr;
  VoxelConeTracingRenderPass* voxel_cone_tracing_pass = nullptr;

  Shader* ping_shader = nullptr;
  Shader* pong_shader = nullptr;

  // Bilateral upsampling shader pass
  uint32_t gl_vao = 0;
  uint32_t gl_ping_fbo = 0;
  uint32_t gl_ping_out_texture = 0;
  uint32_t gl_ping_out_texture_unit = 0;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif // BILATERAL_UPSAMPLING_RENDERPASS
