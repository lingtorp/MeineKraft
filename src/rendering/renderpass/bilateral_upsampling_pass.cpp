#include "bilateral_upsampling_pass.hpp"

#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../math/vector.hpp"
#include "../../rendering/primitives.hpp"
#include "../../util/filesystem.hpp"

#include "downsample_pass.hpp"
#include "gbuffer_pass.hpp"
#include "voxel_cone_tracing_pass.hpp"

#ifdef WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif

bool BilateralUpsamplingRenderPass::setup(Renderer* render) {
  const Resolution screen = render->screen;

  ping_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                           Filesystem::base + "shaders/bilateral-upsampling.frag.glsl");

  // NOTE: Include order matters
  const std::string include0 = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
  const std::string include1 = Filesystem::read_file(Filesystem::base + "shaders/bilateral-filtering-utils.glsl");
  ping_shader->add(include1);
  ping_shader->add(include0);

  const auto [ok, msg] = ping_shader->compile();
  if (!ok) {
    Log::error(msg);
    return false;
  }

  // Ping buffer
  {
    const uint32_t program = ping_shader->gl_program;
    glUseProgram(program);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    GLuint gl_vbo = 0;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

    glGenFramebuffers(1, &gl_ping_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_ping_fbo);

    gl_ping_out_texture_unit = render->get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_ping_out_texture_unit);

    glGenTextures(1, &gl_ping_out_texture);
    glBindTexture(GL_TEXTURE_2D, gl_ping_out_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_ping_out_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_ping_out_texture, -1, "Bilateral upsampling ping output texture");

    const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(std::size(attachments), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Bilateral upsampling ping FBO not complete");
      return false;
    }
  }

  return true;
}

bool BilateralUpsamplingRenderPass::render(Renderer* render) {
  const RenderState state = render->state;
  const Resolution screen = render->screen;

  render->pass_started("Bilateral upsampling pass");

  // NOTE: Joint bilateral upsampling of a filtered low-res noisy signal with high-res 'robust' signal(s)
  const auto bilateral_upsampling_pass = [&](const uint32_t in_texture, const uint32_t in_texture_unit) {
    // Ping - upsamples
    {
      const auto program = ping_shader->gl_program;
      glUseProgram(program);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_ping_fbo);

      glTextureParameteri(in_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTextureParameteri(in_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glTextureParameteri(downsample_pass->gl_normal_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTextureParameteri(downsample_pass->gl_normal_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      glTextureParameteri(downsample_pass->gl_position_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTextureParameteri(downsample_pass->gl_position_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glTextureParameteri(downsample_pass->gl_depth_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTextureParameteri(downsample_pass->gl_depth_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glUniform1i(glGetUniformLocation(program, "uPosition_weight"), state.bilateral_upsample.position_weight);
      glUniform1i(glGetUniformLocation(program, "uPosition_high_res"), gbuffer_pass->gl_position_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uPosition_low_res"), downsample_pass->gl_position_texture_unit);

      glUniform1i(glGetUniformLocation(program, "uNormal_weight"), state.bilateral_upsample.normal_weight);
      glUniform1i(glGetUniformLocation(program, "uNormal_high_res"), gbuffer_pass->gl_geometric_normal_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uNormal_low_res"), downsample_pass->gl_normal_texture_unit);

      glUniform1i(glGetUniformLocation(program, "uTangent"), gbuffer_pass->gl_tangent_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gbuffer_pass->gl_tangent_normal_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uNormal_mapping"), state.bilateral_upsample.normal_mapping);

      glUniform1i(glGetUniformLocation(program, "uDepth_weight"), state.bilateral_upsample.depth_weight);
      glUniform1i(glGetUniformLocation(program, "uDepth_high_res"), gbuffer_pass->gl_depth_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uDepth_low_res"), downsample_pass->gl_depth_texture_unit);

      // Input - low res
      const float div = state.lighting.downsample_modifier;
      const Vec2f input_pixel_size = Vec2f(1.0f / (screen.width * div), 1.0f / (screen.height * div));
      glUniform2fv(glGetUniformLocation(program, "uInput_pixel_size"), 1, &input_pixel_size.x);

      const Vec2f input_texture_size = Vec2f(screen.width / div, screen.height / div);
      glUniform2fv(glGetUniformLocation(program, "uInput_texture_size"), 1, &input_texture_size.x);

      // Output - high res
      const Vec2f output_pixel_size = Vec2f(1.0f / screen.width, 1.0f / screen.height);
      glUniform2fv(glGetUniformLocation(program, "uOutput_pixel_size"), 1, &output_pixel_size.x);

      const Vec2f output_texture_size = Vec2f(screen.width, screen.height);
      glUniform2fv(glGetUniformLocation(program, "uOutput_texture_size"), 1, &output_texture_size.x);

      glUniform1i(glGetUniformLocation(program, "uInput"), in_texture_unit);
      // glUniform1i(glGetUniformLocation(program, "uOutput"), 0); // NOTE: Default to 0 in shader

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Pong - copies the color attacment of ping's FBO
    glCopyTextureSubImage2D(in_texture, 0, 0, 0, 0, 0, screen.width, screen.height);

    glTextureParameteri(in_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(in_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  };

  // Declare input & output texture
  if (state.bilateral_upsample.ambient)  { bilateral_upsampling_pass(voxel_cone_tracing_pass->gl_ambient_radiance_texture,  voxel_cone_tracing_pass->gl_ambient_radiance_texture_unit);  }
  if (state.bilateral_upsample.indirect) { bilateral_upsampling_pass(voxel_cone_tracing_pass->gl_indirect_radiance_texture, voxel_cone_tracing_pass->gl_indirect_radiance_texture_unit); }
  if (state.bilateral_upsample.specular) { bilateral_upsampling_pass(voxel_cone_tracing_pass->gl_specular_radiance_texture, voxel_cone_tracing_pass->gl_specular_radiance_texture_unit); }

  render->pass_ended();

  return true;
}
