#ifndef VOXEL_CONE_TRACING_RENDERPASS_HPP
#define VOXEL_CONE_TRACING_RENDERPASS_HPP

#include "renderpass.hpp"
#include "../primitives.hpp"

#include <stdint.h>

struct Shader;
struct GbufferRenderPass;

struct VoxelConeTracingRenderPass: public RenderPass {
  /// Renderpass Dependencies
  GbufferRenderPass* gbuffer_pass = nullptr;

  Shader* shader = nullptr;

  // Rasterization based VCT pass related
  Shader* vct_shader = nullptr;
  uint32_t gl_vct_fbo = 0;
  uint32_t gl_vct_vao = 0;

  uint32_t gl_vct_diffuse_cones_ssbo = 0;
  uint8_t* gl_vct_diffuse_cones_ssbo_ptr = nullptr;

  // Indirect radiance from GI
  uint32_t gl_indirect_radiance_texture_unit = 0;
  uint32_t gl_indirect_radiance_texture = 0;
  // Ambient radiance from GI
  uint32_t gl_ambient_radiance_texture_unit = 0;
  uint32_t gl_ambient_radiance_texture = 0;
  // Specular radiance from GI
  uint32_t gl_specular_radiance_texture_unit = 0;
  uint32_t gl_specular_radiance_texture = 0;


  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};


#endif // VOXEL_CONE_TRACING_RENDERPASS_HPP
