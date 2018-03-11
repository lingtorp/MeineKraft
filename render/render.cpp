#include "render.h"
#include <GL/glew.h>
#include <random>
#include "camera.h"
#include "graphicsbatch.h"
#include "../util/filemonitor.h"
#include "../util/filesystem.h"
#include "debug_opengl.h"
#include "rendercomponent.h"
#include "meshmanager.h"

/// Column major - Camera combined rotation matrix (y, x) & translation matrix
Mat4<float> Renderer::FPSViewRH(Vec3<float> eye, float pitch, float yaw) {
  static constexpr float rad = M_PI / 180;
  float cosPitch = cosf(pitch * rad);
  float sinPitch = sinf(pitch * rad);
  float cosYaw = cosf(yaw * rad);
  float sinYaw = sinf(yaw * rad);
  auto xaxis = Vec3<float>{cosYaw, 0, -sinYaw};
  auto yaxis = Vec3<float>{sinYaw * sinPitch, cosPitch, cosYaw * sinPitch};
  auto zaxis = Vec3<float>{sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw};
  Mat4<float> matrix;
  matrix[0][0] = xaxis.x;
  matrix[0][1] = yaxis.x;
  matrix[0][2] = zaxis.x;
  matrix[0][3] = 0.0f;
  matrix[1][0] = xaxis.y;
  matrix[1][1] = yaxis.y;
  matrix[1][2] = zaxis.y;
  matrix[1][3] = 0.0f;
  matrix[2][0] = xaxis.z;
  matrix[2][1] = yaxis.z;
  matrix[2][2] = zaxis.z;
  matrix[2][3] = 0.0f;
  matrix[3][0] = -xaxis.dot(eye);
  matrix[3][1] = -yaxis.dot(eye);
  matrix[3][2] = -zaxis.dot(eye); // GLM says no minus , other's say minus
  matrix[3][3] = 1.0f;
  return matrix;
}

/// A.k.a perspective matrix
Mat4<float> gen_projection_matrix(float z_near, float z_far, float fov, float aspect) {
  const float rad = M_PI / 180;
  float tanHalf = tanf(fov * rad / 2);
  float a = 1 / (tanHalf * aspect);
  float b = 1 / tanHalf;
  float c = -(z_far + z_near) / (z_far - z_near);
  float d = -(2 * z_far * z_near) / (z_far - z_near);
  Mat4<float> matrix;
  matrix[0] = {a, 0.0f, 0.0f, 0.0f};
  matrix[1] = {0.0f, b, 0.0f, 0.0f};
  matrix[2] = {0.0f, 0.0f, c, d};
  matrix[3] = {0.0f, 0.0f, -1.0f, 0.0f};
  return matrix;
}

Renderer::Renderer(): projection_matrix(Mat4<float>()), state{}, graphics_batches{},
                    shader_file_monitor(new FileMonitor{}), lights{}, mesh_manager{new MeshManager{}} {
  glewExperimental = (GLboolean) true;
  glewInit();
  
  PointLight light{Vec3<float>{0.0, 15.0, 0.0}};
  lights.push_back(light);
  light = PointLight{Vec3<float>{0.0, 18.0, 0.0}};
  lights.push_back(light);

  Transform transform;
  transform.current_position = light.position;
  transform.repeat = true;
  transformations.push_back(transform);
  
  int screen_width = 1280; // TODO: Move this into uniforms
  int screen_height = 720;
  
  int32_t max_texture_units;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
  SDL_Log("Max texture units: %u", max_texture_units);
  
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
  
  // Global diffuse + specular (albedo) buffer
  gl_albedo_texture_unit = max_texture_units - 8;
  glActiveTexture(GL_TEXTURE0 + gl_albedo_texture_unit);
  glGenTextures(1, &gl_albedo_texture);
  glBindTexture(GL_TEXTURE_2D, gl_albedo_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gl_albedo_texture, 0);
  
  uint32_t depth_attachments[3] = { GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT2 };
  glDrawBuffers(std::size(depth_attachments), depth_attachments);
  
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("Lightning framebuffer status not complete.");
  }
  
  /// Depth shader
  depth_shader = new Shader{Filesystem::base + "shaders/std/depth-vertex.glsl", Filesystem::base + "shaders/std/depth-fragment.glsl"};
  std::string err_msg;
  bool success;
  std::tie(success, err_msg) = depth_shader->compile();
  if (!success) {
    SDL_Log("Shader compilation failed; %s", err_msg.c_str());
  }
  
  /// Stencil framebuffer
  glGenFramebuffers(1, &gl_stencil_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_stencil_fbo);
  
  glGenRenderbuffers(1, &gl_stencil_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, gl_stencil_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screen_width, screen_height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_stencil_rbo);
  
  glDrawBuffer(GL_NONE);
  
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("Stencil framebuffer status not complete.");
  }
  
  stencil_shader = new Shader{Filesystem::base + "shaders/std/stencil.vs", Filesystem::base + "shaders/std/stencil.fs"};
  std::tie(success, err_msg) = stencil_shader->compile();
  if (!success) {
    SDL_Log("Stencil shader compilation failed; %s", err_msg.c_str());
  }
  
  /// Global SSAO framebuffer
  glGenFramebuffers(1, &gl_ssao_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_ssao_fbo);
  
  gl_ssao_texture_unit = max_texture_units - 4;
  glActiveTexture(GL_TEXTURE0 + gl_ssao_texture_unit);
  glGenTextures(1, &gl_ssao_texture);
  glBindTexture(GL_TEXTURE_2D, gl_ssao_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, screen_width, screen_height, 0, GL_RED, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_ssao_texture, 0);
  
  uint32_t ssao_attachments[1] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(std::size(ssao_attachments), ssao_attachments);
  
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("SSAO framebuffer status not complete.");
  }
  
  /// SSAO Shader
  ssao_shader = new Shader{Filesystem::base + "shaders/std/ssao-vertex.glsl", Filesystem::base + "shaders/std/ssao-fragment.glsl"};
  std::tie(success, err_msg) = ssao_shader->compile();
  if (!success) {
    SDL_Log("Shader compilation failed; %s", err_msg.c_str());
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
  gl_ssao_noise_texture_unit = max_texture_units - 2;
  glActiveTexture(GL_TEXTURE0 + gl_ssao_noise_texture_unit);
  glBindTexture(GL_TEXTURE_2D, gl_ssao_noise_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 8, 8, 0, GL_RGB, GL_FLOAT, ssao_noise.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  
  /// Blur pass
  glGenFramebuffers(1, &gl_blur_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_blur_fbo);
  
  blur_shader = new Shader{Filesystem::base + "shaders/std/blur.vs", Filesystem::base + "shaders/std/blur.fs"};
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
  glDrawBuffers(std::size(blur_attachments), blur_attachments);
  
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("Blur framebuffer status not complete.");
  }
  
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
  
  /// Lightning pass shader
  const auto vertex_shader   = Filesystem::base + "shaders/std/vertex-shader.glsl";
  const auto fragment_shader = Filesystem::base + "shaders/std/fragment-shader.glsl";
  lightning_shader = new Shader{vertex_shader, fragment_shader};
  lightning_shader->add("#define FLAG_BLINN_PHONG_SHADING \n");
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
  
  /// SSAO setup
  {
    auto program = ssao_shader->gl_program;
    glGenVertexArrays(1, &gl_ssao_vao);
    glBindVertexArray(gl_ssao_vao);
    
    GLuint ssao_vbo;
    glGenBuffers(1, &ssao_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ssao_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  }
  
  // Blur pass setup
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
  
  /// Stencil pass setup
  {
    auto program = stencil_shader->gl_program;
    glGenVertexArrays(1, &gl_stencil_vao);
    glBindVertexArray(gl_stencil_vao);
  
    GLuint gl_vbo;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sphere.byte_size_of_vertices(), sphere.vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, position));
  
    // Buffer for all the model matrices
    glGenBuffers(1, &gl_stencil_pointlight_models_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, gl_stencil_pointlight_models_buffer_object);
  
    auto model_attrib = glGetAttribLocation(program, "model");
    for (int i = 0; i < 4; i++) {
      glVertexAttribPointer(model_attrib + i, 4, GL_FLOAT, GL_FALSE, sizeof(Mat4<float>), (const void *) (sizeof(float) * i * 4));
      glEnableVertexAttribArray(model_attrib + i);
      glVertexAttribDivisor(model_attrib + i, 1);
    }
  
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.byte_size_of_indices(), sphere.indices.data(), GL_STATIC_DRAW);
  }
  
  /// Point light pass setup
  {
    auto program = lightning_shader->gl_program;
    glGenVertexArrays(1, &gl_lightning_vao);
    glBindVertexArray(gl_lightning_vao);
    
    GLuint gl_vbo;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sphere.byte_size_of_vertices(), sphere.vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, position));
  
    // Buffer for all the model matrices
    glGenBuffers(1, &gl_pointlight_models_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, gl_pointlight_models_buffer_object);
  
    auto model_attrib = glGetAttribLocation(program, "model");
    for (int i = 0; i < 4; i++) {
      glVertexAttribPointer(model_attrib + i, 4, GL_FLOAT, GL_FALSE, sizeof(Mat4<float>), (const void *) (sizeof(float) * i * 4));
      glEnableVertexAttribArray(model_attrib + i);
      glVertexAttribDivisor(model_attrib + i, 1);
    }
  
    GLuint EBO;
    glGenBuffers(1, &EBO);     glDrawBuffer(GL_NONE);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.byte_size_of_indices(), sphere.indices.data(), GL_STATIC_DRAW);
  }

  /// Camera
  const auto position  = Vec3<float>{0.0f, 20.0f, 0.0f};  // cam position
  const auto direction = Vec3<float>{0.0f, 0.0f, -1.0f};  // position of where the cam is looking
  const auto world_up  = Vec3<float>{0.0f, 1.0f, 0.0f};   // world up
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

  auto camera_view = FPSViewRH(camera->position, camera->pitch, camera->yaw);

  if (animate_light) {
    // TODO: Move this kind of comp. into seperate thread or something
    for (auto &transform : transformations) { transform.update(delta); }
    auto temp = camera_view * transformations[0].current_position; // FIXME: Transforms are not updating their Entities..
    lights[0].position = Vec3<float>{temp};
  }
  
  /// Geometry pass
  {
    glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Always update the depth buffer with the new values
    for (const auto& batch : graphics_batches) {
      auto program = depth_shader->gl_program;
      glBindVertexArray(batch.gl_depth_vao);
      glUseProgram(program);
      glUniformMatrix4fv(glGetUniformLocation(program, "camera_view"), 1, GL_FALSE, camera_view.data());
      glUniform1i(glGetUniformLocation(program, "diffuse"), batch.gl_diffuse_texture_unit);
      glUniform1i(glGetUniformLocation(program, "specular"), batch.gl_specular_texture_unit);
  
      std::vector<Mat4<float>> model_buffer{};
      Mat4<float> model{};
      for (const auto& component : batch.components) {
        component->update(); // Copy all graphics state
        
        model = model.translate(component->graphics_state.position);
        model = model.scale(component->graphics_state.scale);
        model_buffer.push_back(model.transpose());
      }
      glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_models_buffer_object);
      glBufferData(GL_ARRAY_BUFFER, model_buffer.size() * sizeof(Mat4<float>), model_buffer.data(), GL_DYNAMIC_DRAW);
      glDrawElementsInstanced(GL_TRIANGLES, batch.mesh.indices.size(), GL_UNSIGNED_INT, nullptr, model_buffer.size());
      /// Update render stats
      state.entities += model_buffer.size();
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
  }
  
  /// SSAO pass
  {
    auto program = ssao_shader->gl_program;
    glBindVertexArray(gl_ssao_vao);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "noise_sampler"), gl_ssao_noise_texture_unit);
    glUniform1i(glGetUniformLocation(program, "normal_sampler"), gl_normal_texture_unit);
    glUniform3fv(glGetUniformLocation(program, "ssao_samples"), ssao_samples.size(), &ssao_samples[0].x);
    glUniform1i(glGetUniformLocation(program, "num_ssao_samples"), ssao_num_samples);
    glUniform1f(glGetUniformLocation(program, "ssao_kernel_radius"), ssao_kernel_radius);
    glUniform1f(glGetUniformLocation(program, "ssao_power"), ssao_power);
    glUniform1f(glGetUniformLocation(program, "ssao_bias"), ssao_bias);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_ssao_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  /// Blur pass
  {
    auto program = blur_shader->gl_program;
    glBindVertexArray(gl_blur_vao);
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "camera_view"), 1, GL_FALSE, camera_view.data());
    glUniform1i(glGetUniformLocation(program, "input_sampler"), gl_ssao_texture_unit);
    glUniform1i(glGetUniformLocation(program, "blur_size"), 2);
    glUniform1f(glGetUniformLocation(program, "blur_factor"), ssao_blur_factor);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_blur_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  
  /// Stencil pass
  {
    // Copy depth + stencil from geometry pass
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_depth_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl_stencil_fbo);
    auto mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    auto filter = GL_NEAREST;
    glBlitFramebuffer(0, 0, screen_width, screen_height, 0, 0, screen_width, screen_height, mask, filter);
    
    auto program = stencil_shader->gl_program;
    glBindFramebuffer(GL_FRAMEBUFFER, gl_stencil_fbo);
    glDrawBuffer(GL_NONE); // Do not draw into color buffer
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
  
    // Setup depth + stencil operations
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Disable depth writes
    glDisable(GL_CULL_FACE);
    
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    
    glBindVertexArray(gl_stencil_vao);
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "camera_view"), 1, GL_FALSE, camera_view.data());
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection_matrix.data());
    std::vector<Mat4<float>> model_buffer;
    Mat4<float> model{};
    for (const auto& light : lights) {
      model = model.translate(light.position);
      model = model.scale(light.radius);
      model_buffer.push_back(model.transpose());
    }
    glBindBuffer(GL_ARRAY_BUFFER, gl_stencil_pointlight_models_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, model_buffer.size() * sizeof(Mat4<float>), model_buffer.data(), GL_DYNAMIC_DRAW);
    glDrawElementsInstanced(GL_TRIANGLES, sphere.indices.size(), GL_UNSIGNED_INT, nullptr, model_buffer.size());
  }
  
  /// Point lighting pass
  {
    // Copy depth + stencil from stencil pass
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_stencil_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    auto mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    auto filter = GL_NEAREST;
    glBlitFramebuffer(0, 0, screen_width, screen_height, 0, 0, screen_width, screen_height, mask, filter);

    auto program = lightning_shader->gl_program;
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Reset framebuffer to default
    glEnable(GL_STENCIL_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glBindVertexArray(gl_lightning_vao);
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "camera_view"), 1, GL_FALSE, camera_view.data());
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection_matrix.data());
  
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CCW);
    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    
    /// Update Light data for the batch
    PointLight& light = lights[0];
    glUniform3fv(glGetUniformLocation(program, "light.color"), 1, &light.color.r);
    glUniform3fv(glGetUniformLocation(program, "light.ambient_intensity"), 1, &light.ambient_intensity.x);
    glUniform3fv(glGetUniformLocation(program, "light.diffuse_intensity"), 1, &light.diffuse_intensity.x);
    glUniform3fv(glGetUniformLocation(program, "light.specular_intensity"), 1, &light.specular_intensity.x);
    glUniform3fv(glGetUniformLocation(program, "light.position"), 1, &light.position.x);
    glUniform1f(glGetUniformLocation(program, "light.constant"), light.constant);
    glUniform1f(glGetUniformLocation(program, "light.linear"), light.linear);
    glUniform1f(glGetUniformLocation(program, "light.quadratic"), light.quadratic);
  
    float c = light.constant; float l = light.linear; float q = light.quadratic;
    float max_brightness = std::fmaxf(std::fmaxf(light.color.r, light.color.g), light.color.b);
    light.radius = (-l + std::sqrt(l * l - 4 * q * (c - (256.0f / 5.0f) * max_brightness))) / (2.0f * q);
    glUniform1f(glGetUniformLocation(program, "light.radius"), light.radius);
  
    /// Update uniforms
    glUniform1i(glGetUniformLocation(program, "normal_sampler"), gl_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "depth_sampler"), gl_depth_texture_unit);
    if (ssao_blur_enabled) {
      glUniform1i(glGetUniformLocation(program, "ssao_sampler"), gl_blur_texture_unit);
    } else {
      glUniform1i(glGetUniformLocation(program, "ssao_sampler"), gl_ssao_texture_unit);
    }
    glUniform1i(glGetUniformLocation(program, "position_sampler"), gl_position_texture_unit);
    glUniform1f(glGetUniformLocation(program, "screen_width"), screen_width);
    glUniform1f(glGetUniformLocation(program, "screen_height"), screen_height);
    glUniform1i(glGetUniformLocation(program, "lightning_enabled"), lightning_enabled);
    glUniform1i(glGetUniformLocation(program, "diffuse_sampler"), gl_albedo_texture_unit);
    glUniform1f(glGetUniformLocation(program, "specular_power"), specular_power);
    glUniform1i(glGetUniformLocation(program, "blinn_phong"), blinn_phong_shading);
    
    std::vector<Mat4<float>> model_buffer;
    for (const auto& light : lights) {
      Mat4<float> model{};
      model = model.translate(light.position);
      model = model.scale(light.radius);
      model_buffer.push_back(model.transpose());
    }
    glBindBuffer(GL_ARRAY_BUFFER, gl_pointlight_models_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, model_buffer.size() * sizeof(Mat4<float>), model_buffer.data(), GL_DYNAMIC_DRAW);
    glDrawElementsInstanced(GL_TRIANGLES, sphere.indices.size(), GL_UNSIGNED_INT, nullptr, model_buffer.size());
    
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
  }
  log_gl_error();
  state.graphic_batches = graphics_batches.size();
}

Renderer::~Renderer() {
  // TODO: Clear up all the GraphicsObjects
  shader_file_monitor->end_monitor();
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

void Renderer::update_projection_matrix(float fov) {
  int height, width;
  SDL_GL_GetDrawableSize(this->window, &width, &height);
  float aspect = (float) width / (float) height;
  this->projection_matrix = gen_projection_matrix(1.0, 10.0, fov, aspect);
  glViewport(0, 0, width, height); // Update OpenGL viewport
  glUseProgram(depth_shader->gl_program);
  glUniformMatrix4fv(glGetUniformLocation(depth_shader->gl_program, "projection"), 1, GL_FALSE, projection_matrix.data());
  // TODO: Adjust all the pass textures sizes
}

void Renderer::link_batch(GraphicsBatch& batch) {
  auto vertices = batch.mesh.to_floats();
  
  /// Geometry pass setup
  {
    auto program = depth_shader->gl_program;
    glGenVertexArrays(1, &batch.gl_depth_vao);
    glBindVertexArray(batch.gl_depth_vao);
    
    GLuint depth_vbo;
    glGenBuffers(1, &depth_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, depth_vbo);
    glBufferData(GL_ARRAY_BUFFER, batch.mesh.byte_size_of_vertices(), vertices.data(), GL_DYNAMIC_DRAW); // FIXME: Should be static draw since mesh is not changing?
    
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
    
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, batch.mesh.byte_size_of_indices(), batch.mesh.indices.data(), GL_STATIC_DRAW);
    
    glUseProgram(program); // Must use the program object before accessing uniforms!
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection_matrix.data());
  }
}

uint64_t Renderer::add_to_batch(RenderComponent* comp) {
  auto mesh_id = comp->graphics_state.mesh_id;
  auto& g_state = comp->graphics_state;
  // TODO: Add components to batches
  
  GraphicsBatch batch{mesh_id};
  batch.mesh = mesh_manager->mesh_from_id(mesh_id);
  link_batch(batch);
  
  if (g_state.diffuse_texture.used) {
    uint32_t buffer_size = 3; // # textures to hold
    batch.init_buffer(&batch.gl_diffuse_texture_array, g_state.diffuse_texture.gl_texture_type, batch.gl_diffuse_texture_unit, buffer_size);
    batch.gl_diffuse_texture_type = g_state.diffuse_texture.gl_texture_type;
  
    /// Assign layer index to the latest the texture and increment
    g_state.diffuse_texture.layer_idx = batch.diffuse_textures_count++;
    /// Add to all the known texture ids in the batch
    batch.texture_ids.push_back(g_state.diffuse_texture.id);
    /// Update the mapping from texture id to layer idx
    batch.layer_idxs[g_state.diffuse_texture.id] = g_state.diffuse_texture.layer_idx;
  
    /// Load all the GState's textures
    RawTexture& texture = g_state.diffuse_texture.data;
    /// Upload the texture to OpenGL
    glActiveTexture(GL_TEXTURE0 + batch.gl_diffuse_texture_unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, batch.gl_diffuse_texture_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                    0,                     // Mipmap number
                    0, 0, g_state.diffuse_texture.layer_idx * 1, // xoffset, yoffset, zoffset = layer face
                    texture.width, texture.height, 1,           // width, height, depth = faces
                    GL_RGB,                // format
                    GL_UNSIGNED_BYTE,      // type
                    texture.pixels);       // pointer to data
  }
  
  if (g_state.specular_texture.used) {
    uint32_t buffer_size = 3; // # textures to hold
    batch.init_buffer(&batch.gl_specular_texture_array, g_state.specular_texture.gl_texture_type, batch.gl_specular_texture_unit, buffer_size);
    batch.gl_specular_texture_type = g_state.specular_texture.gl_texture_type;
  
    /// Assign layer index to the latest the texture and increment
    g_state.specular_texture.layer_idx = batch.specular_textures_count++;
    /// Add to all the known texture ids in the batch
    batch.texture_ids.push_back(g_state.specular_texture.id);
    /// Update the mapping from texture id to layer idx
    batch.layer_idxs[g_state.specular_texture.id] = g_state.specular_texture.layer_idx;
  
    /// Load all the GState's textures
    RawTexture& texture = g_state.specular_texture.data;
    /// Upload the texture to OpenGL
    glActiveTexture(GL_TEXTURE0 + batch.gl_specular_texture_unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, batch.gl_specular_texture_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                    0,                     // Mipmap number
                    0, 0, g_state.specular_texture.layer_idx * 1, // xoffset, yoffset, zoffset = layer face
                    texture.width, texture.height, 1,           // width, height, depth = faces
                    GL_RGB,                // format
                    GL_UNSIGNED_BYTE,      // type
                    texture.pixels);       // pointer to data
  }
  
  batch.components.push_back(comp);
  graphics_batches.push_back(batch);
  
  batch.id = graphics_batches.size(); // TODO: Return real ID
  return batch.id;
}