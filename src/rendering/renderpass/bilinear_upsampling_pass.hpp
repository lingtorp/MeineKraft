#ifndef  BILINEAR_UPSAMPLING_RENDERPASS_HPP
#define  BILINEAR_UPSAMPLING_RENDERPASS_HPP

#include "renderpass.hpp"

#include <stdint.h>

struct Shader;
struct VoxelConeTracingRenderPass;

struct BilinearUpsamplingRenderPass: public RenderPass {
  VoxelConeTracingRenderPass* voxel_cone_tracing_pass = nullptr;

  Shader* shader = nullptr;

  // Bilinear upsampling pass
  Shader* bilinear_upsampling_shader = nullptr;
  uint32_t gl_bilinear_upsampling_fbo = 0;
  uint32_t gl_bilinear_upsampling_texture = 0;
  uint32_t gl_bilinear_upsampling_texture_unit = 0;
  uint32_t gl_ping_out_texture = 0;
  uint32_t gl_ping_out_texture_unit = 0;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif //  BILINEAR_UPSAMPLING_RENDERPASS_HPP
