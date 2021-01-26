#include "voxelization_pass.hpp"

#include "../graphicsbatch.hpp"
#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../math/vector.hpp"
#include "../../rendering/primitives.hpp"
#include "../../util/filesystem.hpp"
#include "../../nodes/model.hpp"
#include "gbuffer_pass.hpp"
#include "directionalshadow_pass.hpp"

#include <GL/glew.h>

bool VoxelizationRenderPass::setup(Renderer* render) {
  shader = new Shader(Filesystem::base + "shaders/voxelization.vert",
                      Filesystem::base + "shaders/voxelization.geom",
                      Filesystem::base + "shaders/voxelization.frag");

  const std::string includes = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
  shader->add(includes);

  const auto [ok, err_msg] = shader->compile();
  if (!ok) {
    Log::error("Voxelization shader error: " + err_msg);
    return false;
  }

  glGenFramebuffers(1, &gl_voxelization_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_voxelization_fbo);
  glObjectLabel(GL_FRAMEBUFFER, gl_voxelization_fbo, -1, "Voxelization FBO");

  for (size_t i = 0; i < Renderer::NUM_CLIPMAPS; i++) {
    // Global radiance voxel buffer
    render->gl_voxel_radiance_image_units[i] = render->get_next_free_image_unit();
    render->gl_voxel_radiance_texture_units[i] = render->get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + render->gl_voxel_radiance_texture_units[i]);
    glGenTextures(1, &render->gl_voxel_radiance_textures[i]);
    glBindTexture(GL_TEXTURE_3D, render->gl_voxel_radiance_textures[i]);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, render->clipmaps.size[i], render->clipmaps.size[i], render->clipmaps.size[i]);
    glBindImageTexture(render->gl_voxel_radiance_image_units[i], render->gl_voxel_radiance_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    const std::string radiance_object_label = "Clipmap #" + std::to_string(i) + " radiance texture";
    glObjectLabel(GL_TEXTURE, render->gl_voxel_radiance_textures[i], -1, radiance_object_label.c_str());

    // Global opacity voxel buffer
    render->gl_voxel_opacity_image_units[i] = render->get_next_free_image_unit();
    render->gl_voxel_opacity_texture_units[i] = render->get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + render->gl_voxel_opacity_texture_units[i]);
    glGenTextures(1, &render->gl_voxel_opacity_textures[i]);
    glBindTexture(GL_TEXTURE_3D, render->gl_voxel_opacity_textures[i]);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, render->clipmaps.size[i], render->clipmaps.size[i], render->clipmaps.size[i]);
    glBindImageTexture(render->gl_voxel_opacity_image_units[i], render->gl_voxel_opacity_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    const std::string opacity_object_label = "Clipmap #" + std::to_string(i) + " opacity texture";
    glObjectLabel(GL_TEXTURE, render->gl_voxel_opacity_textures[i], -1, opacity_object_label.c_str());
  }

  // FIXME: Reuse shadowmap depth attachment for FBO completeness (disabled anyway)
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_pass->gl_shadowmapping_texture, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("Voxelization FBO not complete");
    return false;
  }

  return true;
}

bool VoxelizationRenderPass::render(Renderer* render) {
  const Resolution screen = render->screen;
  const RenderState state = render->state;
  const auto NUM_CLIPMAPS = Renderer::NUM_CLIPMAPS;
  const Scene* scene = render->scene;

  render->pass_started("Voxelization pass");

  for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
    glClearTexImage(render->gl_voxel_radiance_textures[i], 0, GL_RGBA, GL_FLOAT, nullptr);
    glClearTexImage(render->gl_voxel_opacity_textures[i], 0, GL_RGBA, GL_FLOAT, nullptr);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, gl_voxelization_fbo);

  const auto program = shader->gl_program;
  glUseProgram(program);

  // Orthogonal projection along +z-axis
  // TODO: Precompute these - add notification when changed and precompute the new ones
  glm::mat4 orthos[NUM_CLIPMAPS];
  for (size_t i = 0; i < Renderer::NUM_CLIPMAPS; i++) {
    orthos[i] = render->orthographic_projection(render->clipmaps.aabb[i]);
  }
  glUniformMatrix4fv(glGetUniformLocation(program, "uOrthos"), NUM_CLIPMAPS, GL_FALSE, glm::value_ptr(orthos[0]));

  // TODO: Precompute these - add notification when changed and precompute the new ones
  float scaling_factors[NUM_CLIPMAPS];
  for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
    scaling_factors[i] = 1.0f / render->clipmaps.aabb[i].max_axis();
  }
  glUniform1fv(glGetUniformLocation(program, "uScaling_factors"), NUM_CLIPMAPS, scaling_factors);

  // TODO: Precompute these - add notification when changed and precompute the new ones
  Vec3f aabb_centers[NUM_CLIPMAPS];
  for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
    aabb_centers[i] = render->clipmaps.aabb[i].center();
  }
  glUniform3fv(glGetUniformLocation(program, "uAABB_centers"), NUM_CLIPMAPS, &aabb_centers[0].x);
  glUniform3fv(glGetUniformLocation(program, "uCamera_position"), 1, &scene->camera.position.x);

  // Shadowmapping
  glUniform1f(glGetUniformLocation(program, "uShadow_bias"), state.shadow.bias);
  glUniform3fv(glGetUniformLocation(program, "uDirectional_light_direction"), 1, &scene->directional_light.direction.x);
  glUniform3fv(glGetUniformLocation(program, "uDirectional_light_intensity"), 1, &scene->directional_light.intensity.x);
  glUniformMatrix4fv(glGetUniformLocation(program, "uLight_space_transform"), 1, GL_FALSE, glm::value_ptr(shadow_pass->light_space_transform));
  glUniform1i(glGetUniformLocation(program, "uShadowmap"), shadow_pass->gl_shadowmapping_texture_unit);
  glUniform1iv(glGetUniformLocation(program, "uClipmap_sizes"), NUM_CLIPMAPS, render->clipmaps.size);
  glUniform1i(glGetUniformLocation(program, "uConservative_rasterization_enabled"), state.voxelization.conservative_rasterization);

  for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
    glBindImageTexture(render->gl_voxel_radiance_image_units[i], render->gl_voxel_radiance_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(render->gl_voxel_opacity_image_units[i], render->gl_voxel_opacity_textures[i], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
  }
  glUniform1iv(glGetUniformLocation(program, "uVoxel_radiance"), NUM_CLIPMAPS, render->gl_voxel_radiance_image_units);
  glUniform1iv(glGetUniformLocation(program, "uVoxel_opacity"), NUM_CLIPMAPS, render->gl_voxel_opacity_image_units);

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  // TODO: Precompute these - add notification when changed and precompute the new ones
  Vec4f viewports[NUM_CLIPMAPS];
  for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
    viewports[i] = Vec4f(0.0f, 0.0f, render->clipmaps.size[i], render->clipmaps.size[i]);
  }
  glViewportArrayv(0, NUM_CLIPMAPS, &viewports[0].x);

  for (size_t i = 0; i < render->graphics_batches.size(); i++) {
    const auto &batch = render->graphics_batches[i];
    glBindVertexArray(batch.gl_voxelization_vao);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.gl_ibo); // GL_DRAW_INDIRECT_BUFFER is global context state

    glActiveTexture(GL_TEXTURE0 + batch.gl_diffuse_texture_unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, batch.gl_diffuse_texture_array);
    glUniform1i(glGetUniformLocation(program, "uDiffuse"), batch.gl_diffuse_texture_unit);

    glActiveTexture(GL_TEXTURE0 + batch.gl_emissive_texture_unit);
    glBindTexture(GL_TEXTURE_2D, batch.gl_emissive_texture);
    glUniform1i(glGetUniformLocation(program, "uEmissive"), batch.gl_emissive_texture_unit);

    const uint32_t gl_models_binding_point = 2; // Defaults to 2 in geometry.vert shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_models_binding_point, batch.gl_depth_model_buffer);

    const uint32_t gl_material_binding_point = 3; // Defaults to 3 in geometry.frag shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_material_binding_point, batch.gl_material_buffer);

    const uint64_t draw_cmd_offset = batch.gl_curr_ibo_idx * sizeof(DrawElementsIndirectCommand);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void *)draw_cmd_offset, 1, sizeof(DrawElementsIndirectCommand));
  }

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Due to incoherent mem. access need to sync read and usage of voxel data

  // Restore modified global state
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDepthMask(GL_TRUE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glViewport(0, 0, screen.width, screen.height);

  render->pass_ended();

  return false;
}
