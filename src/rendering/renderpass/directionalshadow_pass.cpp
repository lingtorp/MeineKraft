#include "directionalshadow_pass.hpp"

#include "../graphicsbatch.hpp"
#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../math/vector.hpp"
#include "../../rendering/primitives.hpp"
#include "../../util/filesystem.hpp"


#ifdef WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif

bool DirectionalShadowRenderPass::setup(Renderer* render) {
  glGenFramebuffers(1, &gl_shadowmapping_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_shadowmapping_fbo);
  glObjectLabel(GL_FRAMEBUFFER, gl_shadowmapping_fbo, -1, "Shadowmap FBO");

  gl_shadowmapping_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_shadowmapping_texture_unit); // FIXME: These are not neccessary when creating the texture only when they are used
  glGenTextures(1, &gl_shadowmapping_texture);
  glBindTexture(GL_TEXTURE_2D, gl_shadowmapping_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, render->state.shadow.SHADOWMAP_W, render->state.shadow.SHADOWMAP_H);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_shadowmapping_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_shadowmapping_texture, -1, "Shadowmap texture");
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("Directional shadow mapping FBO not complete");
    return false;
  }

  shadowmapping_shader = new Shader(Filesystem::base + "shaders/shadowmapping.vert",
                                    Filesystem::base + "shaders/shadowmapping.frag");
  const auto [ok, err_msg] = shadowmapping_shader->compile();
  if (!ok) {
    Log::error("Shadowmapping shader error: " + err_msg);
    return false;
  }

  return true;
}

bool DirectionalShadowRenderPass::render(Renderer* render) {
  render->pass_started("Directional shadow mapping pass");

  glCullFace(GL_FRONT);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_shadowmapping_fbo);
  glViewport(0, 0, render->state.shadow.SHADOWMAP_W, render->state.shadow.SHADOWMAP_H);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT); // Always update the depth buffer with the new values
  const uint32_t program = shadowmapping_shader->gl_program;
  glUseProgram(program);
  glUniformMatrix4fv(glGetUniformLocation(program, "uLight_space_transform"), 1, GL_FALSE, glm::value_ptr(light_space_transform));

  for (size_t i = 0; i < render->graphics_batches.size(); i++) {
    const auto& batch = render->graphics_batches[i];
    glBindVertexArray(batch.gl_shadowmapping_vao);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.gl_ibo); // GL_DRAW_INDIRECT_BUFFER is global context state

    const uint32_t gl_models_binding_point = 2; // Defaults to 2 in geometry.vert shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_models_binding_point, batch.gl_depth_model_buffer);

    const uint64_t draw_cmd_offset = batch.gl_curr_ibo_idx * sizeof(DrawElementsIndirectCommand);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*) draw_cmd_offset, 1, sizeof(DrawElementsIndirectCommand));
  }

  glViewport(0, 0, render->screen.width, render->screen.height);
  glCullFace(GL_BACK);

  render->pass_ended();


  return true;
}
