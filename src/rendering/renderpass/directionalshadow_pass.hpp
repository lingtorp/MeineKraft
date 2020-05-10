#ifndef DIRECTIONAL_SHADOW_RENDERPASS_HPP
#define DIRECTIONAL_SHADOW_RENDERPASS_HPP

#include "renderpass.hpp"

#include <stdint.h>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Shader;
struct Renderer;

struct DirectionalShadowRenderPass: public RenderPass {
  /// Directional shadow mapping related
  Shader* shadowmapping_shader = nullptr;
  uint32_t gl_shadowmapping_fbo = 0;
  uint32_t gl_shadowmapping_texture = 0;
  uint32_t gl_shadowmapping_texture_unit = 0;

  glm::mat4 light_space_transform;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);

};


#endif // DIRECTIONAL_SHADOW_RENDERPASS_HPP
