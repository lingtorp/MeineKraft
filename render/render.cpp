#include "render.h"

#include <random>
#include <iostream>
#include <fstream>

#ifdef WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif 

#include "camera.h"
#include "graphicsbatch.h"
#include "../util/filesystem.h"
#include "debug_opengl.h"
#include "rendercomponent.h"
#include "meshmanager.h"

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>

static const float SCREEN_WIDTH  = 1280.0f;
static const float SCREEN_HEIGHT = 720.0f;

class RenderPass {
public:
  Shader shader;
  // Global buffers needs to be accessable from the Renderer
  virtual bool setup(Renderer& renderer) const = 0;
  virtual bool start() const = 0;
  virtual bool end() const = 0;
  virtual bool teardown() const = 0;
};

class SSAOPass : RenderPass {
  /// SSAO
  uint32_t ssao_num_samples = 64;
  float ssao_kernel_radius = 1.0f;
  float ssao_power = 1.0f;
  float ssao_bias = 0.0025;
  float ssao_blur_factor = 16.0f;
  bool  ssao_blur_enabled = false;

  /// SSAO pass related
  uint32_t gl_ssao_fbo;
  uint32_t gl_ssao_texture;
  uint32_t gl_ssao_texture_unit;
  uint32_t gl_ssao_vao;

  uint32_t gl_ssao_noise_texture;
  uint32_t gl_ssao_noise_texture_unit;

  std::vector<Vec3<float>> ssao_samples;

  bool setup(Renderer& renderer) {
    const bool setup_successful = true;
    // Allocate all the OpenGL stuff
    // - Inputs, Outputs, objects
    // Compile the Shader
    // gl_variable = glCreate(...)
    // gl_shader_variable = glGetLocation(program, "variablename")
    /// Global SSAO framebuffer
    glGenFramebuffers(1, &gl_ssao_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_ssao_fbo);

    gl_ssao_texture_unit = renderer.get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_ssao_texture_unit);
    glGenTextures(1, &gl_ssao_texture);
    glBindTexture(GL_TEXTURE_2D, gl_ssao_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_ssao_texture, 0);

    uint32_t ssao_attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, ssao_attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      SDL_Log("SSAO framebuffer status not complete.");
      return !setup_successful;
    }

    /// SSAO Shader
    shader = Shader{Filesystem::base + "shaders/ssao.vert", Filesystem::base + "shaders/ssao.frag"};
    bool success = false;
    std::string err_msg;
    std::tie(success, err_msg) = shader.compile();
    if (!success) {
      SDL_Log("Shader compilation failed; %s", err_msg.c_str());
      return !setup_successful;
    }

    /// SSAO noise
    std::uniform_real_distribution<float> random(-1.0f, 1.0f);
    std::default_random_engine gen;
    std::vector<Vec3<float>> ssao_noise;
    for (int i = 0; i < 64; i++) {
      auto noise = Vec3<float>{random(gen), random(gen), 0.0f};
      noise.normalize();
      ssao_noise.push_back(noise);
    }

    glGenTextures(1, &gl_ssao_noise_texture);
    gl_ssao_noise_texture_unit = renderer.get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_ssao_noise_texture_unit);
    glBindTexture(GL_TEXTURE_2D, gl_ssao_noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 8, 8, 0, GL_RGB, GL_FLOAT, ssao_noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /// Create SSAO sample sphere/kernel
    {
      std::uniform_real_distribution<float> random(0.0f, 1.0f);
      std::default_random_engine gen;

      for (size_t i = 0; i < ssao_num_samples; i++) {
        Vec3<float> sample_point = {
            random(gen) * 2.0f - 1.0f, // [-1.0, 1.0]
            random(gen) * 2.0f - 1.0f,
            random(gen)
        };
        sample_point.normalize();
        // Spread the samples inside the hemisphere to fall closer to the origin
        float scale = float(i) / float(ssao_num_samples);
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample_point *= scale;
        ssao_samples.push_back(sample_point);
      }
    }

    /// Fullscreen quad in NDC
    float quad[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    /// SSAO setup
    {
      auto program = shader.gl_program;
      glGenVertexArrays(1, &gl_ssao_vao);
      glBindVertexArray(gl_ssao_vao);

      GLuint ssao_vbo;
      glGenBuffers(1, &ssao_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, ssao_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);
      glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
      glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    }

    return setup_successful;
  }

  bool start() {
    // Push the latest values to the shader
    // glUniform1f(gl_shader_variable, value)
    return true;
  }
};

/// Pass handling code - used for debuggging at this moment
void pass_started(const std::string& msg) {
#if defined(__LINUX__) || defined(WIN32)
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, msg.c_str());
#endif
}

void pass_ended() {
#if defined(__LINUX__) || defined(WIN32)
  glPopDebugGroup();
#endif
}

/// A.k.a perspective matrix
Mat4<float> gen_projection_matrix(float z_near, float z_far, float fov, float aspect) {
  const float rad = M_PI / 180.0f;
  float tanHalf = tanf(fov * rad / 2);
  float a = 1.0f / (tanHalf * aspect);
  float b = 1.0f / tanHalf;
  float c = -(z_far + z_near) / (z_far - z_near);
  float d = -(2 * z_far * z_near) / (z_far - z_near);
  Mat4<float> matrix;
  matrix[0] = {a, 0.0f, 0.0f, 0.0f};
  matrix[1] = {0.0f, b, 0.0f, 0.0f};
  matrix[2] = {0.0f, 0.0f, c, d};
  matrix[3] = {0.0f, 0.0f, -1.0f, 0.0f};
  return matrix;
}

uint32_t Renderer::get_next_free_texture_unit() {
  int32_t max_texture_units;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
  next_texture_unit++;
  if (next_texture_unit >= max_texture_units) {
    SDL_Log("Reached max texture units: %u", max_texture_units);
    exit(1);
  }
  return next_texture_unit;
};

Renderer::Renderer(): graphics_batches{} {
  glewExperimental = (GLboolean) true;
  glewInit();

  // OpenGL debug output
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(gl_debug_callback, 0);
  glDebugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);

  int screen_width = 1280; // TODO: Move this into uniforms
  int screen_height = 720;

  // TODO: Remove
  int32_t max_texture_units;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);

  /// Global geometry pass framebuffer
  glGenFramebuffers(1, &gl_depth_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);

  gl_depth_texture_unit = max_texture_units - 1;
  glActiveTexture(GL_TEXTURE0 + gl_depth_texture_unit);
  glGenTextures(1, &gl_depth_texture);
  glBindTexture(GL_TEXTURE_2D, gl_depth_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  // glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT); // Default value (intention only to read depth values from texture)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, screen_width, screen_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gl_depth_texture, 0);

  // Global normal buffer
  gl_normal_texture_unit = max_texture_units - 3;
  glActiveTexture(GL_TEXTURE0 + gl_normal_texture_unit);
  glGenTextures(1, &gl_normal_texture);
  glBindTexture(GL_TEXTURE_2D, gl_normal_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen_width, screen_height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_normal_texture, 0);

  // Global position buffer
  gl_position_texture_unit = max_texture_units - 5;
  glActiveTexture(GL_TEXTURE0 + gl_position_texture_unit);
  glGenTextures(1, &gl_position_texture);
  glBindTexture(GL_TEXTURE_2D, gl_position_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen_width, screen_height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gl_position_texture, 0);

  // Global diffuse buffer
  gl_albedo_texture_unit = max_texture_units - 8;
  glActiveTexture(GL_TEXTURE0 + gl_albedo_texture_unit);
  glGenTextures(1, &gl_albedo_texture);
  glBindTexture(GL_TEXTURE_2D, gl_albedo_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gl_albedo_texture, 0);

  uint32_t depth_attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
  glDrawBuffers(3, depth_attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("Lightning framebuffer status not complete.");
  }

  /// Depth shader
  depth_shader = new Shader{Filesystem::base + "shaders/geometry.vert", Filesystem::base + "shaders/geometry.frag"};
  std::string err_msg;
  bool success;
  std::tie(success, err_msg) = depth_shader->compile();
  if (!success) {
    SDL_Log("Shader compilation failed; %s", err_msg.c_str());
  }

  /// Point lightning framebuffer
  glGenFramebuffers(1, &gl_lightning_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_lightning_fbo);

  GLuint gl_lightning_rbo;
  glGenRenderbuffers(1, &gl_lightning_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, gl_lightning_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screen_width, screen_height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_lightning_rbo);

  gl_lightning_texture_unit = max_texture_units - 9;
  glActiveTexture(GL_TEXTURE0 + gl_lightning_texture_unit);
  glGenTextures(1, &gl_lightning_texture);
  glBindTexture(GL_TEXTURE_2D, gl_lightning_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_lightning_texture, 0);

  uint32_t lightning_attachments[1] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(1, lightning_attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("Point lightning framebuffer status not complete.");
  }

  /// Blur pass
  glGenFramebuffers(1, &gl_blur_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_blur_fbo);

  blur_shader = new Shader{Filesystem::base + "shaders/blur.vert", Filesystem::base + "shaders/blur.frag"};
  std::tie(success, err_msg) = blur_shader->compile();
  if (!success) {
    SDL_Log("Blur shader compilation failed; %s", err_msg.c_str());
  }

  gl_blur_texture_unit = max_texture_units - 7;
  glActiveTexture(GL_TEXTURE0 + gl_blur_texture_unit);
  glGenTextures(1, &gl_blur_texture);
  glBindTexture(GL_TEXTURE_2D, gl_blur_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, screen_width, screen_height, 0, GL_RED, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_blur_texture, 0);

  uint32_t blur_attachments[1] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(1, blur_attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("Blur framebuffer status not complete.");
  }

  /// Lightning pass shader
  const auto vertex_shader   = Filesystem::base + "shaders/lightning.vert";
  const auto fragment_shader = Filesystem::base + "shaders/lightning.frag";
  lightning_shader = new Shader{vertex_shader, fragment_shader};
  std::tie(success, err_msg) = lightning_shader->compile();
  if (!success) {
    SDL_Log("Lightning shader compilation failed; %s", err_msg.c_str());
  }

  /// Fullscreen quad in NDC
  float quad[] = {
    // positions        // texture Coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
  };

  /// Blur pass setup
  {
    auto program = blur_shader->gl_program;
    glGenVertexArrays(1, &gl_blur_vao);
    glBindVertexArray(gl_blur_vao);

    GLuint blur_vbo;
    glGenBuffers(1, &blur_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, blur_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  }

  /// Point light pass setup
  {
    auto program = lightning_shader->gl_program;
    glGenVertexArrays(1, &gl_lightning_vao);
    glBindVertexArray(gl_lightning_vao);

    GLuint gl_vbo;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  }

  /// Camera
  const auto position  = Vec3<float>{5.0f, 0.0f, 5.0f};  
  const auto direction = Vec3<float>{0.0f, 0.0f, -1.0f};  
  const auto world_up  = Vec3<float>{0.0f, 1.0f, 0.0f};  
  camera = new Camera(position, direction, world_up);
}

bool Renderer::point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes) {
  const auto left_plane = planes[0]; const auto right_plane = planes[1];
  const auto top_plane  = planes[2]; const auto bot_plane   = planes[3];
  const auto near_plane = planes[4]; const auto far_plane   = planes[5];
  const auto dist_l = left_plane.distance_to_point(point);
  const auto dist_r = right_plane.distance_to_point(point);
  const auto dist_t = top_plane.distance_to_point(point);
  const auto dist_b = bot_plane.distance_to_point(point);
  const auto dist_n = near_plane.distance_to_point(point);
  const auto dist_f = far_plane.distance_to_point(point);
  return dist_l < 0 && dist_r < 0 && dist_t < 0 && dist_b < 0 && dist_n < 0 && dist_f < 0;
}

void Renderer::render(uint32_t delta) {
  /// Reset render stats
  state = RenderState();

  glm::mat4 camera_transform = camera->transform();

  /// Geometry pass
  pass_started("Geometry pass");
  {
    glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Always update the depth buffer with the new values
    for (const auto& batch : graphics_batches) {
      const auto program = depth_shader->gl_program;
      glUseProgram(program);
      glUniformMatrix4fv(glGetUniformLocation(program, "camera_view"), 1, GL_FALSE, glm::value_ptr(camera_transform));

      std::vector<Mat4<float>> model_buffer{};
      std::vector<uint32_t> diffuse_textures_idx{};
      for (const auto& component : batch.components) {
        component->update(); // Copy all graphics state
        
        Mat4<float> model{};
        model = model.translate(component->graphics_state.position);
        model = model.scale(component->graphics_state.scale);
        model_buffer.push_back(model.transpose());
        
        diffuse_textures_idx.push_back(component->graphics_state.diffuse_texture.layer_idx);
      }
      
      glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_models_buffer_object);
      glBufferData(GL_ARRAY_BUFFER, model_buffer.size() * sizeof(Mat4<float>), model_buffer.data(), GL_DYNAMIC_DRAW);
      
      glBindBuffer(GL_ARRAY_BUFFER, batch.gl_diffuse_textures_layer_idx);
      glBufferData(GL_ARRAY_BUFFER, diffuse_textures_idx.size() * sizeof(uint32_t), diffuse_textures_idx.data(), GL_DYNAMIC_DRAW);

      glBindVertexArray(batch.gl_depth_vao);
      glDrawElementsInstanced(GL_TRIANGLES, batch.mesh.indices.size(), GL_UNSIGNED_INT, nullptr, model_buffer.size());
      
      state.entities += model_buffer.size();
    }
  }
  pass_ended();

  pass_started("Point lightning pass");
  {
    const auto program = lightning_shader->gl_program;
    glBindFramebuffer(GL_FRAMEBUFFER, gl_lightning_fbo);

    glBindVertexArray(gl_lightning_vao);
    glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "diffuse_sampler"), gl_albedo_texture_unit);
    glUniform1i(glGetUniformLocation(program, "normal_sampler"), gl_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "position_sampler"), gl_position_texture_unit);
    glUniform1f(glGetUniformLocation(program, "screen_width"), screen_width);
    glUniform1f(glGetUniformLocation(program, "screen_height"), screen_height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  pass_ended();

  /// Copy final pass into default FBO
  pass_started("Final blit pass");
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_lightning_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    auto mask = GL_COLOR_BUFFER_BIT;
    auto filter = GL_NEAREST;
    glBlitFramebuffer(0, 0, screen_width, screen_height, 0, 0, screen_width, screen_height, mask, filter);
  }
  pass_ended();

  log_gl_error();
  state.graphic_batches = graphics_batches.size();
}

/// Returns the planes from the frustrum matrix in order; {left, right, bottom, top, near, far}
/// See: http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
std::array<Plane<float>, 6> Renderer::extract_planes(Mat4<float> mat) {
  auto planes = std::array<Plane<float>, 6>();
  auto left_plane  = Plane<float>(mat[3][0] + mat[0][0],
                                  mat[3][1] + mat[0][1],
                                  mat[3][2] + mat[0][2],
                                  mat[3][3] + mat[0][3]);

  auto right_plane = Plane<float>(mat[3][0] - mat[0][0],
                                  mat[3][1] - mat[0][1],
                                  mat[3][2] - mat[0][2],
                                  mat[3][3] - mat[0][3]);

  auto bot_plane   = Plane<float>(mat[3][0] + mat[1][0],
                                  mat[3][1] + mat[1][1],
                                  mat[3][2] + mat[1][2],
                                  mat[3][3] + mat[1][3]);

  auto top_plane   = Plane<float>(mat[3][0] - mat[1][0],
                                  mat[3][1] - mat[1][1],
                                  mat[3][2] - mat[1][2],
                                  mat[3][3] - mat[1][3]);

  auto near_plane  = Plane<float>(mat[3][0] + mat[2][0],
                                  mat[3][1] + mat[2][1],
                                  mat[3][2] + mat[2][2],
                                  mat[3][3] + mat[2][3]);

  auto far_plane   = Plane<float>(mat[3][0] - mat[2][0],
                                  mat[3][1] - mat[2][1],
                                  mat[3][2] - mat[2][2],
                                  mat[3][3] - mat[2][3]);
  planes[0] = left_plane; planes[1] = right_plane; planes[2] = bot_plane;
  planes[3] = top_plane;  planes[4] = near_plane;  planes[5] = far_plane;
  return planes;
}

void Renderer::update_projection_matrix(const float fov) {
  float aspect = (float) screen_width / (float) screen_height;
  this->projection_matrix = gen_projection_matrix(1.0, 10.0, fov, aspect);
  glViewport(0, 0, screen_width, screen_height); // Update OpenGL viewport
  // TODO: Adjust all the passes textures sizes
  // TODO: Adjust the projections matrix in all passes that use it?
  glUseProgram(depth_shader->gl_program);
  glUniformMatrix4fv(glGetUniformLocation(depth_shader->gl_program, "projection"), 1, GL_FALSE, projection_matrix.data());
}

void Renderer::link_batch(GraphicsBatch& batch) {
  /// Geometry pass setup
  {
    auto program = depth_shader->gl_program;
    glGenVertexArrays(1, &batch.gl_depth_vao);
    glBindVertexArray(batch.gl_depth_vao);

    glGenBuffers(1, &batch.gl_depth_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_vbo);
    const auto vertices = batch.mesh.to_floats(); 
    glBufferData(GL_ARRAY_BUFFER, batch.mesh.byte_size_of_vertices(), vertices.data(), GL_STATIC_DRAW);

    auto position_attrib = glGetAttribLocation(program, "position");
    glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, position));
    glEnableVertexAttribArray(position_attrib);

    auto normal_attrib = glGetAttribLocation(program, "normal");
    glVertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, normal));
    glEnableVertexAttribArray(normal_attrib);

    auto texcoord_attrib = glGetAttribLocation(program, "texcoord");
    glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, texCoord));
    glEnableVertexAttribArray(texcoord_attrib);

    // Buffer for all the model matrices
    glGenBuffers(1, &batch.gl_depth_models_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_models_buffer_object);

    auto model_attrib = glGetAttribLocation(program, "model");
    for (int i = 0; i < 4; i++) {
      glVertexAttribPointer(model_attrib + i, 4, GL_FLOAT, GL_FALSE, sizeof(Mat4<float>), (const void *) (sizeof(float) * i * 4));
      glEnableVertexAttribArray(model_attrib + i);
      glVertexAttribDivisor(model_attrib + i, 1);
    }

    // Buffer for all the diffuse texture indices
    glGenBuffers(1, &batch.gl_diffuse_textures_layer_idx);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_diffuse_textures_layer_idx);
    glVertexAttribIPointer(glGetAttribLocation(program, "diffuse_layer_idx"), 1, GL_UNSIGNED_INT, sizeof(GLint), nullptr);
    glEnableVertexAttribArray(glGetAttribLocation(program, "diffuse_layer_idx"));
    glVertexAttribDivisor(glGetAttribLocation(program, "diffuse_layer_idx"), 1);

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, batch.mesh.byte_size_of_indices(), batch.mesh.indices.data(), GL_STATIC_DRAW);

    glUseProgram(program); // Must use the program object before accessing uniforms!
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection_matrix.data());
    glUniform1i(glGetUniformLocation(program, "diffuse"), batch.gl_diffuse_texture_unit);
  }
}

Renderer::~Renderer() = default;

uint64_t Renderer::add_to_batch(RenderComponent* comp) {
  auto mesh_id = comp->graphics_state.mesh_id;
  auto& g_state = comp->graphics_state;

  for (auto& batch : graphics_batches) {
      if (batch.mesh_id == mesh_id) {
          batch.components.push_back(comp);

          for (const auto& item : batch.layer_idxs) {
            const auto id = item.first; // Texture id
            if (id == g_state.diffuse_texture.id) {
              g_state.diffuse_texture.layer_idx = batch.layer_idxs[id];
              return batch.id;
            }
          }

          /// Expand texture buffer if needed
          if (batch.diffuse_textures_count + 1 > batch.diffuse_textures_capacity) {
            batch.expand_texture_buffer(&batch.gl_diffuse_texture_array, batch.gl_diffuse_texture_type);
          }

          /// Assign layer index to the latest the texture and increment
          g_state.diffuse_texture.layer_idx = batch.diffuse_textures_count++;
          
          /// Update the mapping from texture id to layer idx
          batch.layer_idxs[g_state.diffuse_texture.id] = g_state.diffuse_texture.layer_idx;

          /// Upload the texture to OpenGL
          batch.upload(g_state.diffuse_texture);

          return batch.id;
      }
  }

  GraphicsBatch batch{mesh_id};
  batch.id = graphics_batches.size(); // TODO: Return real ID
  batch.mesh = MeshManager::mesh_from_id(mesh_id);
  link_batch(batch);

  if (g_state.diffuse_texture.used) {
    batch.init_buffer(&batch.gl_diffuse_texture_array, batch.gl_diffuse_texture_unit, g_state.diffuse_texture);

    /// Assign layer index to the latest the texture and increment
    g_state.diffuse_texture.layer_idx = batch.diffuse_textures_count++;

    /// Update the mapping from texture id to layer idx
    batch.layer_idxs[g_state.diffuse_texture.id] = g_state.diffuse_texture.layer_idx;

    /// Upload the texture to OpenGL
    batch.upload(g_state.diffuse_texture);
  }

  batch.components.push_back(comp);
  graphics_batches.push_back(batch);

  return batch.id;
}