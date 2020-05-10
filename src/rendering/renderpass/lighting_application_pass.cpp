#include "lighting_application_pass.hpp"

#include "../graphicsbatch.hpp"
#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../math/vector.hpp"
#include "../../rendering/primitives.hpp"
#include "../renderpass/gbuffer_pass.hpp"
#include "../renderpass/voxel_cone_tracing_pass.hpp"
#include "../../util/filesystem.hpp"

#ifdef WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif

bool LightingApplicationRenderPass::setup(Renderer* render) {
  const Resolution screen = render->screen;

  shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                      Filesystem::base + "shaders/lighting-application.frag.glsl");

  const std::string includes = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
  shader->add(includes);

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

  glGenFramebuffers(1, &gl_lighting_application_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_lighting_application_fbo);

  gl_lighting_application_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_lighting_application_texture_unit);

  glGenTextures(1, &gl_lighting_application_texture);
  glBindTexture(GL_TEXTURE_2D, gl_lighting_application_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_lighting_application_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_lighting_application_texture, -1, "Lighting application texture");

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_lighting_application_texture, 0);

  const uint32_t attachments[1] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(std::size(attachments), attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("Lighting application FBO not complete");
    return false;
  }

  return true;
}

bool LightingApplicationRenderPass::render(Renderer* render) {
  const Resolution screen = render->screen;
  RenderState& state = render->state;

  render->pass_started("Lighting application pass");

  const auto program = shader->gl_program;
  glUseProgram(program);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_lighting_application_fbo);

  const Vec2f pixel_size = Vec2(1.0f / screen.width, 1.0f / screen.height);
  glUniform2fv(glGetUniformLocation(program, "uPixel_size"), 1, &pixel_size.x);

  glUniform1i(glGetUniformLocation(program, "uDiffuse"), gbuffer_pass->gl_diffuse_texture_unit);

  glUniform1i(glGetUniformLocation(program, "uIndirect_radiance"), voxel_cone_tracing_pass->gl_indirect_radiance_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uAmbient_radiance"), voxel_cone_tracing_pass->gl_ambient_radiance_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uSpecular_radiance"), voxel_cone_tracing_pass->gl_specular_radiance_texture_unit);

  glUniform1i(glGetUniformLocation(program, "uEmissive_radiance"), gbuffer_pass->gl_emissive_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uDirect_radiance"), gbuffer_pass->gl_direct_radiance_texture_unit);

  // Applicable radiance values
  const bool ambient_radiance_applicable = state.lighting.downsample_modifier > 1  ? (state.bilateral_upsample.enabled && state.bilateral_upsample.ambient) || (state.bilinear_upsample.enabled && state.bilinear_upsample.ambient) : true;
  glUniform1i(glGetUniformLocation(program, "uAmbient_radiance_applicable"), ambient_radiance_applicable);
  const bool indirect_radiance_applicable = state.lighting.downsample_modifier > 1  ? (state.bilateral_upsample.enabled && state.bilateral_upsample.indirect) || (state.bilinear_upsample.enabled && state.bilinear_upsample.indirect) : true;
  glUniform1i(glGetUniformLocation(program, "uIndirect_radiance_applicable"), indirect_radiance_applicable);
  const bool specular_radiance_applicable = state.lighting.downsample_modifier > 1  ? (state.bilateral_upsample.enabled && state.bilateral_upsample.specular) || (state.bilinear_upsample.enabled && state.bilinear_upsample.specular) : true;
  glUniform1i(glGetUniformLocation(program, "uSpecular_radiance_applicable"), specular_radiance_applicable);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  render->pass_ended();

  return true;
}
