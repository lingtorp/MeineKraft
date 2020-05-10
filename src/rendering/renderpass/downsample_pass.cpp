#include "downsample_pass.hpp"

#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../math/vector.hpp"
#include "../../rendering/primitives.hpp"
#include "../../util/filesystem.hpp"

#include "directionalshadow_pass.hpp"
#include "gbuffer_pass.hpp"

#ifdef WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif

bool DownsampleRenderPass::setup(Renderer* render) {
  const Resolution screen = render->screen;

  /// Downsampled global geometry pass framebuffer
  glGenFramebuffers(1, &gl_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
  glObjectLabel(GL_FRAMEBUFFER, gl_fbo, -1, "GBuffer (downsampled) FBO");

  // Global downsampled position buffer
  gl_position_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_position_texture_unit);
  glGenTextures(1, &gl_position_texture);
  glBindTexture(GL_TEXTURE_2D, gl_position_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_position_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_position_texture, -1, "GBuffer (downsampled) position texture");

  // Global downsampled normal buffer
  gl_normal_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_normal_texture_unit);
  glGenTextures(1, &gl_normal_texture);
  glBindTexture(GL_TEXTURE_2D, gl_normal_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gl_normal_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_normal_texture, -1, "GBuffer (downsampled) normal texture");

  // Global downsampled depth buffer
  gl_depth_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_depth_texture_unit);
  glGenTextures(1, &gl_depth_texture);
  glBindTexture(GL_TEXTURE_2D, gl_depth_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, screen.width, screen.height, 0, GL_RED, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gl_depth_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_depth_texture, -1, "GBuffer (downsampled) depth texture");

  uint32_t attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
  glDrawBuffers(std::size(attachments), attachments);

  // NOTE: Reuse shadowmap depth attachment for FBO completeness (disabled anyway)
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_pass->gl_shadowmapping_texture, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("GBuffer (downsampled) framebuffer status not complete.");
  }

  shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                                          Filesystem::base + "shaders/gbuffer-downsampled.frag.glsl");
  const auto [ok, err_msg] = shader->compile();
  if (!ok) {
    Log::error("Gbuffer (downsampled) shader error: " + err_msg); exit(1);
  }

  const auto program = shader->gl_program;
  glUseProgram(program);

  glGenVertexArrays(1, &gl_vao);
  glBindVertexArray(gl_vao);

  GLuint gl_vbo = 0;
  glGenBuffers(1, &gl_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
  glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  return true;
}

bool DownsampleRenderPass::render(Renderer* render) {
  const float width = render->screen.width;
  const float height = render->screen.height;

  render->pass_started("Downsample pass");

  glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
  glBindVertexArray(gl_vao);

  const auto program = shader->gl_program;
  glUseProgram(program);

  const auto div = render->state.lighting.downsample_modifier;
  const Vec2f input_pixel_size = Vec2(float(div) / (render->screen.width), float(div) / (render->screen.height));
  glUniform2fv(glGetUniformLocation(program, "uInput_pixel_size"), 1, &input_pixel_size.x);

  glUniform1i(glGetUniformLocation(program, "uPosition"), gl_position_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uNormal"), gbuffer_pass->gl_geometric_normal_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uDepth"), gl_depth_texture_unit);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, render->screen.width / div, render->screen.height / div);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glViewport(0, 0, render->screen.width, render->screen.height);

  render->pass_ended();

  return true;
}
