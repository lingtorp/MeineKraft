#include "bilateral_filtering_pass.hpp"

#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../util/filesystem.hpp"
#include "voxel_cone_tracing_pass.hpp"
#include "gbuffer_pass.hpp"

#ifdef WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif

bool BilateralFilteringRenderPass::setup(Renderer* render) {
  const Resolution screen = render->screen;

  ping_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                           Filesystem::base + "shaders/bilateral-filtering.frag.glsl");

  // NOTE: Include order matters
  const std::string include0 = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
  const std::string include1 = Filesystem::read_file(Filesystem::base + "shaders/bilateral-filtering-utils.glsl");
  ping_shader->add(include1);
  ping_shader->add(include0);
  ping_shader->add("#define VERTICAL_STEP_DIR \n");

  const auto [ok, msg] = ping_shader->compile();
  if (!ok) {
    Log::error(msg);
    return false;
  }

  pong_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                           Filesystem::base + "shaders/bilateral-filtering.frag.glsl");

  pong_shader->add(include1);
  pong_shader->add(include0);
  pong_shader->add("#define HORIZONTAL_STEP_DIR \n");

  const auto [ok_pong, msg_pong] = pong_shader->compile();
  if (!ok_pong) {
    Log::error(msg_pong);
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

    glGenFramebuffers(1, &gl_bf_ping_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_bf_ping_fbo);

    gl_bf_ping_out_texture_unit = render->get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_bf_ping_out_texture_unit);

    glGenTextures(1, &gl_bf_ping_out_texture);
    glBindTexture(GL_TEXTURE_2D, gl_bf_ping_out_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_bf_ping_out_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_bf_ping_out_texture, -1, "Bilateral ping output texture");

    const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(std::size(attachments), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Bilateral ping FBO not complete"); exit(-1);
    }
  }

  // Pong buffer
  {
    const uint32_t program = pong_shader->gl_program;
    glUseProgram(program);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    GLuint gl_vbo = 0;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

    glGenFramebuffers(1, &gl_bf_pong_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_bf_pong_fbo);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_bf_ping_out_texture, 0); // NOTE: Use w/e temp. texture for FBO completeness

    const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(std::size(attachments), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Bilateral filtering pong FBO not complete");
      return false;
    }
  }

  return true;
}

bool BilateralFilteringRenderPass::render(Renderer* render) {
  RenderState& state = render->state;
  const Resolution screen = render->screen;

  state.bilateral_filtering.kernel = gaussian_1d_kernel(state.bilateral_filtering.spatial_kernel_sigma, state.bilateral_filtering.spatial_kernel_radius);
  render->pass_started("Bilateral filtering pass");

  // NOTE: Joint bilateral filtering of a noisy signal with one or more 'robust' signals
  const auto bilateral_filtering_pass = [&](const uint32_t in_texture, const uint32_t in_texture_unit, const float div = 1.0f) {
    // TODO: Reduce to one single shader, reuse that one twice instead
    // Ping
    {
      const auto program = bf_ping_shader->gl_program;
      glUseProgram(program);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_bf_ping_fbo);

      glUniform1i(glGetUniformLocation(program, "uPosition_weight"), state.bilateral_filtering.position_weight);
      glUniform1i(glGetUniformLocation(program, "uPosition"), gbuffer_pass->gl_position_texture_unit);
      glUniform1f(glGetUniformLocation(program, "uPosition_sigma"), state.bilateral_filtering.position_sigma);
      glUniform1i(glGetUniformLocation(program, "uNormal_weight"), state.bilateral_filtering.normal_weight);
      glUniform1i(glGetUniformLocation(program, "uNormal"), gbuffer_pass->gl_geometric_normal_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uTangent"), gbuffer_pass->gl_tangent_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gbuffer_pass->gl_tangent_normal_texture_unit);
      glUniform1f(glGetUniformLocation(program, "uNormal_sigma"), state.bilateral_filtering.normal_sigma);
      glUniform1i(glGetUniformLocation(program, "uDepth_weight"), state.bilateral_filtering.depth_weight);
      glUniform1i(glGetUniformLocation(program, "uDepth"), gbuffer_pass->gl_depth_texture_unit);
      glUniform1f(glGetUniformLocation(program, "uDepth_sigma"), state.bilateral_filtering.depth_sigma);

      const Vec2f pixel_size = Vec2f(1.0f / screen.width, 1.0f / screen.height);
      glUniform2fv(glGetUniformLocation(program, "uInput_pixel_size"), 1, &pixel_size.x);
      glUniform2fv(glGetUniformLocation(program, "uOutput_pixel_size"), 1, &pixel_size.x);

      glUniform1ui(glGetUniformLocation(program, "uKernel_dim"), state.bilateral_filtering.kernel.size());
      glUniform1fv(glGetUniformLocation(program, "uKernel"), state.bilateral_filtering.kernel.size(), state.bilateral_filtering.kernel.data());

      glUniform1i(glGetUniformLocation(program, "uInput"), in_texture_unit);
      // glUniform1i(glGetUniformLocation(program, "uOutput"), 0); // NOTE: Default to 0 in shader

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glViewport(0, 0, screen.width / div, screen.height / div);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Pong
    {
      const auto program = bf_pong_shader->gl_program;
      glUseProgram(program);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_bf_pong_fbo);

      glUniform1i(glGetUniformLocation(program, "uPosition_weight"), state.bilateral_filtering.position_weight);
      glUniform1i(glGetUniformLocation(program, "uPosition"), gbuffer_pass->gl_position_texture_unit);
      glUniform1f(glGetUniformLocation(program, "uPosition_sigma"), state.bilateral_filtering.position_sigma);
      glUniform1i(glGetUniformLocation(program, "uNormal_weight"), state.bilateral_filtering.normal_weight);
      glUniform1i(glGetUniformLocation(program, "uNormal"), gbuffer_pass->gl_geometric_normal_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gbuffer_pass->gl_tangent_normal_texture_unit);
      glUniform1i(glGetUniformLocation(program, "uTangent"), gbuffer_pass->gl_tangent_texture_unit);
      glUniform1f(glGetUniformLocation(program, "uNormal_sigma"), state.bilateral_filtering.normal_sigma);
      glUniform1i(glGetUniformLocation(program, "uDepth_weight"), state.bilateral_filtering.depth_weight);
      glUniform1i(glGetUniformLocation(program, "uDepth"), gbuffer_pass->gl_depth_texture_unit);
      glUniform1f(glGetUniformLocation(program, "uDepth_sigma"), state.bilateral_filtering.depth_sigma);

      const Vec2f pixel_size = Vec2f(1.0f / screen.width, 1.0f / screen.height);
      glUniform2fv(glGetUniformLocation(program, "uInput_pixel_size"), 1, &pixel_size.x);
      glUniform2fv(glGetUniformLocation(program, "uOutput_pixel_size"), 1, &pixel_size.x);

      glUniform1ui(glGetUniformLocation(program, "uKernel_dim"), state.bilateral_filtering.kernel.size());
      glUniform1fv(glGetUniformLocation(program, "uKernel"), state.bilateral_filtering.kernel.size(), state.bilateral_filtering.kernel.data());

      glUniform1i(glGetUniformLocation(program, "uInput"), gl_bf_ping_out_texture_unit);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, in_texture, 0);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glViewport(0, 0, screen.width / div, screen.height / div);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glViewport(0, 0, screen.width, screen.height);
  };

  // // Pre-filtering screenshot
  // const auto get_texture = [&](const uint32_t gl_texture) -> Vec3f* {
  //   const size_t buf_size = sizeof(Vec3f) * screen.width * screen.height;
  //   Vec3f *pixels = (Vec3f*) calloc(buf_size, 1);
  //   glGetTextureImage(gl_texture, 0, GL_RGB, GL_FLOAT, buf_size, (void*) pixels);
  //   return pixels;
  // };

  // Vec3f* ambient_radiance_pixels = nullptr;
  // Vec3f* indirect_radiance_pixels = nullptr;
  // Vec3f* specular_radiance_pixels = nullptr;
  // if (state.bilateral_filtering.pixel_diff) {
  //   if (state.bilateral_filtering.ambient)  { ambient_radiance_pixels = get_texture(gl_ambient_radiance_texture); };
  //   if (state.bilateral_filtering.indirect) { indirect_radiance_pixels = get_texture(gl_indirect_radiance_texture); };
  //   if (state.bilateral_filtering.specular) { specular_radiance_pixels = get_texture(gl_specular_radiance_texture); };
  // }

  // // Declare input & output texture
  // const float div = state.lighting.downsample_modifier;
  // if (state.bilateral_filtering.ambient)  { bilateral_filtering_pass(gl_ambient_radiance_texture,  gl_ambient_radiance_texture_unit, div);  }
  // if (state.bilateral_filtering.indirect) { bilateral_filtering_pass(gl_indirect_radiance_texture, gl_indirect_radiance_texture_unit, div); }
  // if (state.bilateral_filtering.specular) { bilateral_filtering_pass(gl_specular_radiance_texture, gl_specular_radiance_texture_unit, div); }
  // if (state.bilateral_filtering.direct)   { bilateral_filtering_pass(gl_direct_radiance_texture,   gl_direct_radiance_texture_unit);   }

  // const auto save_pixel_diff = [&](const std::string& filename, const Vec3f* pre, const Vec3f* post, const TextureFormat fmt = TextureFormat::RGB32F) {
  //   const std::string pre_fp  = Filesystem::save_image_as_ppm(Filesystem::tmp + "pre-"  + filename, pre, screen.width, screen.height, div, fmt);
  //   const std::string post_fp = Filesystem::save_image_as_ppm(Filesystem::tmp + "post-" + filename, post, screen.width, screen.height, div, fmt);
  //   const std::string out_fp = Filesystem::tmp + "diff-" + filename + ".png";
  //   // FIXME: Compare using grayscale not binary white-black?
  //   const std::string cmd = "compare -highlight-color white " + pre_fp + " " + post_fp + " " + out_fp;
  //   std::system(cmd.c_str());
  //   Log::info("Saved pixel difference: " + out_fp);
  //   delete pre;
  //   delete post;
  // };

  // // Post-filtering screenshot
  // if (state.bilateral_filtering.pixel_diff) {
  //   if (state.bilateral_filtering.ambient)   { save_pixel_diff("ambient-radiance",  ambient_radiance_pixels, get_texture(gl_ambient_radiance_texture), TextureFormat::R32F); };
  //   if (state.bilateral_filtering.indirect)  { save_pixel_diff("indirect-radiance", indirect_radiance_pixels, get_texture(gl_indirect_radiance_texture)); };
  //   if (state.bilateral_filtering.specular)  { save_pixel_diff("specular-radiance", specular_radiance_pixels, get_texture(gl_specular_radiance_texture)); };
  //   state.bilateral_filtering.pixel_diff = false;
  // }

  render->pass_ended();

  return true;
}
