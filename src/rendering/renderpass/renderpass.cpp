// #include "renderpass.hpp"
// #include "../renderer.hpp"
// #include "../util/filesystem.hpp"

// #include <random>
// #include <string>
// #include <vector>
// #include <tuple>

// #ifdef WIN32
// #include <glew.h>
// #else
// #include <GL/glew.h>
// #endif

// /// Blurpass
// bool BlurPass::setup(Renderer& renderer) {
//   /// Blur pass
//   glGenFramebuffers(1, &gl_blur_fbo);
//   glBindFramebuffer(GL_FRAMEBUFFER, gl_blur_fbo);

//   shader = Shader{ Filesystem::base + "shaders/blur.vert", Filesystem::base + "shaders/blur.frag" };
//   bool success = false;
//   std::string err_msg = "";
//   std::tie(success, err_msg) = shader.compile();
//   if (!success) {
//     Log::error("Blur shader compilation failed; " + err_msg);
//     return success;
//   }

//   gl_blur_texture_unit = Renderer::get_next_free_texture_unit();
//   glActiveTexture(GL_TEXTURE0 + gl_blur_texture_unit);
//   glGenTextures(1, &gl_blur_texture);
//   glBindTexture(GL_TEXTURE_2D, gl_blur_texture);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//   glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, renderer.screen.width, renderer.screen.height, 0, GL_RED, GL_FLOAT, nullptr);
//   glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_blur_texture, 0);

//   uint32_t blur_attachments[1] = { GL_COLOR_ATTACHMENT0 };
//   glDrawBuffers(1, blur_attachments);

//   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
//     Log::error("Blur framebuffer status not complete.");
//     return false;
//   }

//   glGenVertexArrays(1, &gl_blur_vao);
//   glBindVertexArray(gl_blur_vao);

//   GLuint blur_vbo;
//   glGenBuffers(1, &blur_vbo);
//   glBindBuffer(GL_ARRAY_BUFFER, blur_vbo);
//   glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);
//   glEnableVertexAttribArray(glGetAttribLocation(shader.gl_program, "position"));
//   glVertexAttribPointer(glGetAttribLocation(shader.gl_program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

//   return success;
// }

// bool BlurPass::render() const {
//   return true;
// }

// /// SSAOPass
// bool SSAOPass::setup(Renderer& renderer) {
//   const bool setup_successful = true;
//   // Allocate all the OpenGL stuff
//   // - Inputs, Outputs, objects
//   // Compile the Shader
//   // gl_variable = glCreate(...)
//   // gl_shader_variable = glGetLocation(program, "variablename")
//   /// Global SSAO framebuffer
//   glGenFramebuffers(1, &gl_ssao_fbo);
//   glBindFramebuffer(GL_FRAMEBUFFER, gl_ssao_fbo);

//   gl_ssao_texture_unit = Renderer::get_next_free_texture_unit();
//   glActiveTexture(GL_TEXTURE0 + gl_ssao_texture_unit);
//   glGenTextures(1, &gl_ssao_texture);
//   glBindTexture(GL_TEXTURE_2D, gl_ssao_texture);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//   glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, renderer.screen.width, renderer.screen.height, 0, GL_RED, GL_FLOAT, nullptr);
//   glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_ssao_texture, 0);

//   uint32_t ssao_attachments[1] = { GL_COLOR_ATTACHMENT0 };
//   glDrawBuffers(1, ssao_attachments);

//   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
//     Log::error("SSAO framebuffer status not complete.");
//     return !setup_successful;
//   }

//   /// SSAO Shader
//   shader = Shader{ Filesystem::base + "shaders/ssao.vert", Filesystem::base + "shaders/ssao.frag" };
//   bool success = false;
//   std::string err_msg;
//   std::tie(success, err_msg) = shader.compile();
//   if (!success) {
//     Log::error("Shader compilation failed; " + err_msg);
//     return !setup_successful;
//   }

//   /// SSAO noise
//   std::uniform_real_distribution<float> random(-1.0f, 1.0f);
//   std::default_random_engine gen;
//   std::vector<Vec3<float>> ssao_noise;
//   for (int i = 0; i < 64; i++) {
//     auto noise = Vec3<float>{ random(gen), random(gen), 0.0f };
//     noise.normalize();
//     ssao_noise.push_back(noise);
//   }

//   glGenTextures(1, &gl_ssao_noise_texture);
//   gl_ssao_noise_texture_unit = renderer.get_next_free_texture_unit();
//   glActiveTexture(GL_TEXTURE0 + gl_ssao_noise_texture_unit);
//   glBindTexture(GL_TEXTURE_2D, gl_ssao_noise_texture);
//   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 8, 8, 0, GL_RGB, GL_FLOAT, ssao_noise.data());
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

//   /// Create SSAO sample sphere/kernel
//   {
//     std::uniform_real_distribution<float> random(0.0f, 1.0f);
//     std::default_random_engine gen;

//     for (size_t i = 0; i < ssao_num_samples; i++) {
//       Vec3<float> sample_point = {
//           random(gen) * 2.0f - 1.0f, // [-1.0, 1.0]
//           random(gen) * 2.0f - 1.0f,
//           random(gen)
//       };
//       sample_point.normalize();
//       // Spread the samples inside the hemisphere to fall closer to the origin
//       float scale = float(i) / float(ssao_num_samples);
//       scale = lerp(0.1f, 1.0f, scale * scale);
//       sample_point *= scale;
//       ssao_samples.push_back(sample_point);
//     }
//   }

//   /// SSAO setup
//   {
//     auto program = shader.gl_program;
//     glGenVertexArrays(1, &gl_ssao_vao);
//     glBindVertexArray(gl_ssao_vao);

//     GLuint ssao_vbo;
//     glGenBuffers(1, &ssao_vbo);
//     glBindBuffer(GL_ARRAY_BUFFER, ssao_vbo);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);
//     glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
//     glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
//   }

//   return setup_successful;
// }

// bool SSAOPass::render() const {
//   // Push the latest values to the shader
//   // glUniform1f(gl_shader_variable, value)
//   return true;
// }
