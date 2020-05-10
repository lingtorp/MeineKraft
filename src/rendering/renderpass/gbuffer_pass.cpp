#include "gbuffer_pass.hpp"

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

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

bool GbufferRenderPass::setup(Renderer* render) {
  const Resolution screen = render->screen;
  const RenderState state = render->state;

  /// Global geometry pass framebuffer
  glGenFramebuffers(1, &gl_depth_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
  glObjectLabel(GL_FRAMEBUFFER, gl_depth_fbo, -1, "GBuffer FBO");

  // Global depth buffer
  gl_depth_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_depth_texture_unit);
  glGenTextures(1, &gl_depth_texture);
  glBindTexture(GL_TEXTURE_2D, gl_depth_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  // glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT); // Default value (intention only to read depth values from texture)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, screen.width, screen.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_depth_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_depth_texture, -1, "GBuffer depth texture");

  // Global normal buffer
  gl_geometric_normal_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_geometric_normal_texture_unit);
  glGenTextures(1, &gl_geometric_normal_texture);
  glBindTexture(GL_TEXTURE_2D, gl_geometric_normal_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_geometric_normal_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_geometric_normal_texture, -1, "GBuffer geometric normal texture");

  // Global position buffer
  gl_position_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_position_texture_unit);
  glGenTextures(1, &gl_position_texture);
  glBindTexture(GL_TEXTURE_2D, gl_position_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gl_position_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_position_texture, -1, "GBuffer position texture");

  // Global diffuse buffer
  gl_diffuse_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_diffuse_texture_unit);
  glGenTextures(1, &gl_diffuse_texture);
  glBindTexture(GL_TEXTURE_2D, gl_diffuse_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gl_diffuse_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_diffuse_texture, -1, "GBuffer diffuse texture");

  // Global PBR parameters buffer
  gl_pbr_parameters_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_pbr_parameters_texture_unit);
  glGenTextures(1, &gl_pbr_parameters_texture);
  glBindTexture(GL_TEXTURE_2D, gl_pbr_parameters_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, gl_pbr_parameters_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_pbr_parameters_texture, -1, "GBuffer PBR parameters texture");

  // Global emissive map
  gl_emissive_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_emissive_texture_unit);
  glGenTextures(1, &gl_emissive_texture);
  glBindTexture(GL_TEXTURE_2D, gl_emissive_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, gl_emissive_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_emissive_texture, -1, "GBuffer emissive texture");

  // Global shading model id
  gl_shading_model_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_shading_model_texture_unit);
  glGenTextures(1, &gl_shading_model_texture);
  glBindTexture(GL_TEXTURE_2D, gl_shading_model_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, screen.width, screen.height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, gl_shading_model_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_shading_model_texture, -1, "GBuffer shading ID texture");

  // Global tangent space normal map
  gl_tangent_normal_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_tangent_normal_texture_unit);
  glGenTextures(1, &gl_tangent_normal_texture);
  glBindTexture(GL_TEXTURE_2D, gl_tangent_normal_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, gl_tangent_normal_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_tangent_normal_texture, -1, "GBuffer tangent normal texture");

  // Global tangent map
  gl_tangent_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_tangent_texture_unit);
  glGenTextures(1, &gl_tangent_texture);
  glBindTexture(GL_TEXTURE_2D, gl_tangent_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT7, gl_tangent_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_tangent_texture, -1, "GBuffer tangent texture");

  uint32_t attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
  glDrawBuffers(std::size(attachments), attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("GBuffer framebuffer status not complete.");
    return false;
  }

  // Global direct radiance map
  gl_direct_radiance_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_direct_radiance_texture_unit);
  glGenTextures(1, &gl_direct_radiance_texture);
  glBindTexture(GL_TEXTURE_2D, gl_direct_radiance_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glObjectLabel(GL_TEXTURE, gl_direct_radiance_texture, -1, "GBuffer direct radiance texture");

  return true;
}

bool GbufferRenderPass::render(Renderer* render) {
  const Resolution screen = render->screen;
  const RenderState state = render->state;

  render->pass_started("Geometry pass");

  glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  for (size_t i = 0; i < render->graphics_batches.size(); i++) {
    const auto& batch = render->graphics_batches[i];
    const auto program = batch.depth_shader.gl_program;
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(render->projection_matrix));
    glBindVertexArray(batch.gl_depth_vao);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.gl_ibo); // GL_DRAW_INDIRECT_BUFFER is global context state

    glUniformMatrix4fv(glGetUniformLocation(program, "camera_view"), 1, GL_FALSE, glm::value_ptr(render->camera_transform));

    const uint32_t gl_models_binding_point = 2; // Defaults to 2 in geometry.vert shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_models_binding_point, batch.gl_depth_model_buffer);

    const uint32_t gl_material_binding_point = 3; // Defaults to 3 in geometry.frag shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_material_binding_point, batch.gl_material_buffer);

    glActiveTexture(GL_TEXTURE0 + batch.gl_diffuse_texture_unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, batch.gl_diffuse_texture_array);

    if (batch.gl_metallic_roughness_texture != 0) {
      glActiveTexture(GL_TEXTURE0 + batch.gl_metallic_roughness_texture_unit); // TODO: Replace with DSA
      glBindTexture(GL_TEXTURE_2D, batch.gl_metallic_roughness_texture);
    }

    if (batch.gl_tangent_normal_texture != 0) {
      glActiveTexture(GL_TEXTURE0 + batch.gl_tangent_normal_texture_unit); // TODO: Replace with DSA
      glBindTexture(GL_TEXTURE_2D, batch.gl_tangent_normal_texture);
    }

    glActiveTexture(GL_TEXTURE0 + batch.gl_emissive_texture_unit); // TODO: Replace with DSA
    glBindTexture(GL_TEXTURE_2D, batch.gl_emissive_texture);

    const uint64_t draw_cmd_offset = batch.gl_curr_ibo_idx * sizeof(DrawElementsIndirectCommand);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*) draw_cmd_offset, 1, sizeof(DrawElementsIndirectCommand));
  }

  render->pass_ended();

  return true;
}
