#ifndef  GBUFFER_RENDERPASS_HPP
#define  GBUFFER_RENDERPASS_HPP

#include "renderpass.hpp"

#include <stdint.h>

// TODO:
// a.k.a Geometry pass
struct GbufferRenderPass:  public RenderPass {
  /// Geometry pass related
  uint32_t gl_depth_fbo = 0;
  uint32_t gl_depth_texture = 0;
  uint32_t gl_depth_texture_unit = 0;

  /// Global buffers
  // Geometric normals
  uint32_t gl_geometric_normal_texture = 0;
  uint32_t gl_geometric_normal_texture_unit = 0;
  //  Tangent space normals
  uint32_t gl_tangent_normal_texture = 0;
  uint32_t gl_tangent_normal_texture_unit = 0;
  // Tangent map
  uint32_t gl_tangent_texture = 0;
  uint32_t gl_tangent_texture_unit = 0;
  // Positions
  uint32_t gl_position_texture = 0;
  uint32_t gl_position_texture_unit = 0;
  // Diffuse
  uint32_t gl_diffuse_texture = 0;
  uint32_t gl_diffuse_texture_unit = 0;
  // PBR Parameters
  uint32_t gl_pbr_parameters_texture = 0;
  uint32_t gl_pbr_parameters_texture_unit = 0;
  // Emissive map
  uint32_t gl_emissive_texture_unit = 0;
  uint32_t gl_emissive_texture = 0;
  // Shading model
  uint32_t gl_shading_model_texture_unit = 0;
  uint32_t gl_shading_model_texture = 0;
  // Direct radiance
  uint32_t gl_direct_radiance_texture_unit = 0;
  uint32_t gl_direct_radiance_texture = 0;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif //  GBUFFER_RENDERPASS_HPP
