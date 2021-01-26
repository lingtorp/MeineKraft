#include "direct_lighting_pass.hpp"

#include "../graphicsbatch.hpp"
#include "../renderer.hpp"
#include "../../nodes/model.hpp"
#include "../shader.hpp"
#include "../../math/vector.hpp"
#include "../../rendering/primitives.hpp"
#include "../renderpass/gbuffer_pass.hpp"
#include "../renderpass/directionalshadow_pass.hpp"
#include "../../util/filesystem.hpp"

#include <GL/glew.h>

bool DirectLightingRenderPass::setup(Renderer* render) {
  shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                      Filesystem::base + "shaders/direct-lighting.frag.glsl");

  const auto [ok, msg] = shader->compile();
  if (!ok) {
    Log::error(msg);
    return false;
  }

  const uint32_t program = shader->gl_program;
  glUseProgram(program);
  glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
  glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  GLuint gl_vbo = 0;
  glGenBuffers(1, &gl_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

  glGenFramebuffers(1, &gl_direct_lighting_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_direct_lighting_fbo);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer_pass->gl_direct_radiance_texture, 0);

  const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(std::size(attachments), attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("Direct lighting FBO not complete");
    return false;
  }

  return true;
}

bool DirectLightingRenderPass::render(Renderer* render) {
  const Resolution screen = render->screen;
  const RenderState state = render->state;
  Scene* scene = render->scene;

  render->pass_started("Direct lighting pass");

  const auto program = shader->gl_program;
  glUseProgram(program);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_direct_lighting_fbo);

  glUniform1ui(glGetUniformLocation(program, "uPCF_samples"), state.shadow.pcf_samples);
  glUniform1f(glGetUniformLocation(program, "uVCT_shadow_cone_aperature"), state.shadow.vct_cone_aperature);
  glUniform1ui(glGetUniformLocation(program, "uShadow_algorithm"), static_cast<uint32_t>(state.shadow.algorithm));
  glUniform1f(glGetUniformLocation(program, "uShadow_bias"), state.shadow.bias);
  glUniform3fv(glGetUniformLocation(program, "uDirectional_light_intensity"), 1, &scene->directional_light.intensity.x);
  glUniform3fv(glGetUniformLocation(program, "uDirectional_light_direction"), 1, &scene->directional_light.direction.x);
  glUniformMatrix4fv(glGetUniformLocation(program, "uLight_space_transform"), 1, GL_FALSE, glm::value_ptr(shadow_pass->light_space_transform));
  glUniform1i(glGetUniformLocation(program, "uShadowmap"), shadow_pass->gl_shadowmapping_texture_unit);
  glUniform1ui(glGetUniformLocation(program, "uShadowmap_width"), state.shadow.SHADOWMAP_W);
  glUniform1ui(glGetUniformLocation(program, "uShadowmap_height"), state.shadow.SHADOWMAP_H);

  glUniform1ui(glGetUniformLocation(program, "uScreen_width"), screen.width);
  glUniform1ui(glGetUniformLocation(program, "uScreen_height"), screen.height);
  glUniform1i(glGetUniformLocation(program, "uPosition"), gbuffer_pass->gl_position_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uNormal"), gbuffer_pass->gl_geometric_normal_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gbuffer_pass->gl_tangent_normal_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uTangent"), gbuffer_pass->gl_tangent_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uNormalmapping"), state.lighting.normalmapping);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  render->pass_ended();

  return true;
}
