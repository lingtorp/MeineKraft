#include "bilinear_upsampling_pass.hpp"

#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../util/filesystem.hpp"
#include "voxel_cone_tracing_pass.hpp"

#include <GL/glew.h>

bool BilinearUpsamplingRenderPass::setup(Renderer* render) {
  const Resolution screen = render->screen;

  shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                      Filesystem::base + "shaders/bilinear-upsampling.frag.glsl");

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

  glGenFramebuffers(1, &gl_bilinear_upsampling_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_bilinear_upsampling_fbo);

  gl_ping_out_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_ping_out_texture_unit);

  glGenTextures(1, &gl_ping_out_texture);
  glBindTexture(GL_TEXTURE_2D, gl_ping_out_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_ping_out_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_ping_out_texture, -1, "Bilinear ping output texture");

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_ping_out_texture, 0);

  const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(std::size(attachments), attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("Bilinear upsampling FBO not complete");
    return false;
  }

  return true;
}

bool BilinearUpsamplingRenderPass::render(Renderer* render) {
  const RenderState state = render->state;
  const Resolution screen = render->screen;

  if (state.bilinear_upsample.enabled && state.lighting.downsample_modifier > 1) {
    render->pass_started("Bilinear upsampling pass");

    // FIXME: Reuses some of Bilateral filtering ping-pong texture resources
    const auto bilinear_upsample = [&](const uint32_t in_texture, const uint32_t in_texture_unit) {
      const auto program = shader->gl_program;
      glUseProgram(program);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_bilinear_upsampling_fbo);

      if (state.bilinear_upsample.nearest_neighbor) {
        glTextureParameteri(in_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(in_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(gl_ping_out_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(gl_ping_out_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      } else {
        glTextureParameteri(in_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(in_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(gl_ping_out_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(gl_ping_out_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }

      // Ping
      {
        const float d = state.lighting.downsample_modifier;
        const Vec2f output_pixel_size = Vec2f(1.0f / (screen.width * d), 1.0f / (screen.height * d));
        glUniform2fv(glGetUniformLocation(program, "uOutput_pixel_size"), 1, &output_pixel_size.x);

        glUniform1i(glGetUniformLocation(program, "uInput"), in_texture_unit);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_ping_out_texture, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      }

      // Pong - copies the color attacment of ping's FBO
      glCopyTextureSubImage2D(in_texture, 0, 0, 0, 0, 0, screen.width, screen.height);
    };

    if (state.bilinear_upsample.ambient)  { bilinear_upsample(voxel_cone_tracing_pass->gl_ambient_radiance_texture,  voxel_cone_tracing_pass->gl_ambient_radiance_texture_unit);   }
    if (state.bilinear_upsample.indirect) { bilinear_upsample(voxel_cone_tracing_pass->gl_indirect_radiance_texture, voxel_cone_tracing_pass->gl_indirect_radiance_texture_unit); }
    if (state.bilinear_upsample.specular) { bilinear_upsample(voxel_cone_tracing_pass->gl_specular_radiance_texture, voxel_cone_tracing_pass->gl_specular_radiance_texture_unit); }

    render->pass_ended();
  }

  return true;
}
