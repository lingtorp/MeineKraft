#include "render.h"
#include <GL/glew.h>
#include <SDL2/SDL_image.h>
#include <array>
#include "../world/world.h"
#include "camera.h"
#include "graphicsbatch.h"
#include "rendercomponent.h"
#include "shader.h"
#include "../util/filemonitor.h"
#include "transform.h"
#include "meshmanager.h"
#include "texturemanager.h"
#include "../util/filesystem.h"
#include "debug_opengl.h"

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

Renderer::Renderer(): DRAW_DISTANCE(200), projection_matrix(Mat4<float>()), state{}, graphics_batches{},
                    shader_file_monitor(std::make_unique<FileMonitor>()), lights{}, mesh_manager{new MeshManager()},
                    texture_manager{TextureManager{}} {
  glewExperimental = (GLboolean) true;
  glewInit();
  
  Light light{Vec3<float>{15.0, 15.0, 15.0}, Color4<float>{0.5, 0.4, 0.8, 1.0}};
  lights.push_back(light);

  Transform transform;
  transform.current_position = light.position;
  transform.repeat = true;
  transformations.push_back(transform);

  /// Light uniform buffer
  glGenBuffers(1, &gl_light_uniform_buffer);
  glBindBuffer(GL_UNIFORM_BUFFER, gl_light_uniform_buffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(Light) * lights.size(), &lights, GL_DYNAMIC_DRAW);
  
  int screen_width = 1280;
  int screen_height = 720;
  
  int32_t max_texture_units;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
  SDL_Log("Max texture units: %u", max_texture_units);
  
  /// Global depth pass framebuffer
  glGenFramebuffers(1, &gl_depth_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
  
  gl_depth_texture_unit = max_texture_units - 1;
  glActiveTexture(GL_TEXTURE0 + gl_depth_texture_unit);
  glGenTextures(1, &gl_depth_texture);
  glBindTexture(GL_TEXTURE_2D, gl_depth_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, screen_width, screen_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_depth_texture, 0);
  
  // Global normal buffer
  gl_normal_texture_unit = max_texture_units - 3;
  glActiveTexture(GL_TEXTURE0 + gl_normal_texture_unit);
  glGenTextures(1, &gl_normal_texture);
  glBindTexture(GL_TEXTURE_2D, gl_normal_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screen_width, screen_height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_normal_texture, 0);
  
  // Global position buffer
  gl_position_texture_unit = max_texture_units - 5;
  glActiveTexture(GL_TEXTURE0 + gl_position_texture_unit);
  glGenTextures(1, &gl_position_texture);
  glBindTexture(GL_TEXTURE_2D, gl_position_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screen_width, screen_height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gl_position_texture, 0);
  
  uint32_t depth_attachments[2] = { GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(2, depth_attachments);
  
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("Framebuffer status not complete.");
  }
  
  /// Depth shader
  depth_shader = new Shader{FileSystem::base + "shaders/std/depth-vertex.glsl", FileSystem::base + "shaders/std/depth-fragment.glsl"};
  std::string err_msg;
  bool success;
  std::tie(success, err_msg) = depth_shader->compile();
  if (!success) {
    SDL_Log("Shader compilation failed; %s", err_msg.c_str());
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
  glDrawBuffers(1, ssao_attachments);
  
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_Log("Framebuffer status not complete.");
  }
  
  /// SSAO Shader
  ssao_shader = new Shader{FileSystem::base + "shaders/std/ssao-vertex.glsl", FileSystem::base + "shaders/std/ssao-fragment.glsl"};
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
  
  /// Camera
  const auto position  = Vec3<float>{0.0f, 20.0f, 0.0f};  // cam position
  const auto direction = Vec3<float>{0.0f, 0.0f, -1.0f};  // position of where the cam is looking
  const auto world_up  = Vec3<float>{0.0f, 1.0f, 0.0f};   // world up
  this->camera = std::make_shared<Camera>(position, direction, world_up);
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

  /// Frustrum planes
  auto camera_view = FPSViewRH(camera->position, camera->pitch, camera->yaw);
  auto frustrum_view = camera_view * projection_matrix; // FIXME: Matrix multiplication is probably defined backwards
  std::array<Plane<float>, 6> planes = extract_planes(frustrum_view.transpose());

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
  glClearColor(0.8f, 0.5f, 0.5f, 1.0f);

  // TODO: Move this kind of comp. into seperate thread or something
  for (auto &transform : transformations) { transform.update(delta); }
  lights[0].position = transformations[0].current_position; // FIXME: Transforms are not updating their Entities..
  
  /// Create SSAO sample sphere/kernel
  std::vector<Vec3<float>> ssao_samples;
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
      sample_point *= random(gen);
      // Spread the samples inside the hemisphere to fall closer to the origin
      float scale = float(i) / float(ssao_num_samples);
      scale = lerp(0.1f, 1.0f, scale * scale);
      sample_point *= scale;
      ssao_samples.push_back(sample_point);
    }
  }
  
  /// SSAO noise
  {
    std::uniform_real_distribution<float> random(-1.0f, 1.0f);
    std::default_random_engine gen;
    std::vector<Vec3<float>> ssao_noise;
    for (int i = 0; i < 64; i++) {
      auto noise = Vec3<float>{random(gen), random(gen), 0.0f};
      noise.normalize();
      ssao_noise.push_back(noise);
    }
    glBindTexture(GL_TEXTURE_2D, gl_ssao_noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 8, 8, 0, GL_RGB, GL_FLOAT, ssao_noise.data());
  }
  
  for (auto& batch : graphics_batches) {
    std::vector<Mat4<float>> model_buffer{};
  
    /// Depth pass
    {
      glBindVertexArray(batch.gl_depth_vao);
      glUseProgram(depth_shader->gl_program);
      glUniformMatrix4fv(batch.gl_depth_camera_view, 1, GL_FALSE, camera_view.data());

      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
      glFrontFace(GL_CCW);
      
      for (auto &component : batch.components) {
        component->update(); // Copy all graphics state
      
        Mat4<float> model{};
        model = model.translate(component->graphics_state.position);
        model = model.scale(component->graphics_state.scale);
        model_buffer.push_back(model.transpose());
      }
      glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_models_buffer_object);
      glBufferData(GL_ARRAY_BUFFER, model_buffer.size() * sizeof(Mat4<float>), model_buffer.data(), GL_DYNAMIC_DRAW);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Always update the depth buffer with the new values
      glDrawElementsInstanced(GL_TRIANGLES, batch.mesh.indices.size(), GL_UNSIGNED_INT, nullptr, model_buffer.size());
    }
    
    /// SSAO pass
    {
      auto program = ssao_shader->gl_program;
      glBindVertexArray(batch.gl_ssao_vao);
      glUseProgram(program);
      glUniformMatrix4fv(glGetUniformLocation(program, "camera_view"), 1, GL_FALSE, camera_view.data());
      // Updates uniforms
      glUniform1i(glGetUniformLocation(program, "depth_sampler"), gl_depth_texture_unit);
      glUniform1i(glGetUniformLocation(program, "noise_sampler"), gl_ssao_noise_texture_unit);
      glUniform1i(glGetUniformLocation(program, "normal_sampler"), gl_normal_texture_unit);
      glUniform3fv(glGetUniformLocation(program, "ssao_samples"), ssao_samples.size(), &ssao_samples[0].x);
      glUniform1i(glGetUniformLocation(program, "num_ssao_samples"), ssao_num_samples);
      glUniform1f(glGetUniformLocation(program, "ssao_kernel_radius"), ssao_kernel_radius);
      glUniform1f(glGetUniformLocation(program, "ssao_power"), ssao_power);
      glUniform1f(glGetUniformLocation(program, "ssao_bias"), ssao_bias);
      glBindBuffer(GL_ARRAY_BUFFER, batch.gl_ssao_models_buffer_object);
      glBufferData(GL_ARRAY_BUFFER, model_buffer.size() * sizeof(Mat4<float>), model_buffer.data(), GL_DYNAMIC_DRAW);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_ssao_fbo);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glDrawElementsInstanced(GL_TRIANGLES, batch.mesh.indices.size(), GL_UNSIGNED_INT, nullptr, model_buffer.size());
    }
    
    /// Remainder pass
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0); // Reset framebuffer to default
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Always update the depth buffer with the new values
    
      glBindVertexArray(batch.gl_VAO);
      glUseProgram(batch.shader.gl_program);
      glUniformMatrix4fv(batch.gl_camera_view, 1, GL_FALSE, camera_view.data());
      glUniform3fv(batch.gl_camera_position, 1, (const GLfloat *) &camera->position);
    
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
      glFrontFace(GL_CCW);
    
      /// Update Light data for the batch
      glBindBuffer(GL_UNIFORM_BUFFER, gl_light_uniform_buffer);
      glBufferData(GL_UNIFORM_BUFFER, sizeof(Light) * lights.size(), lights.data(), GL_DYNAMIC_DRAW);
    
      // Normal texture
      glUniform1i(glGetUniformLocation(batch.shader.gl_program, "normal_sampler"), gl_normal_texture_unit);
      glUniform1i(glGetUniformLocation(batch.shader.gl_program, "depth_sampler"), gl_depth_texture_unit);
      glUniform1i(glGetUniformLocation(batch.shader.gl_program, "ssao_sampler"), gl_ssao_texture_unit);
    
      std::vector<Mat4<float>> buffer{};
      std::vector<uint32_t> diffuse_texture_idxs{};
      for (auto &component : batch.components) {
        /// Add the diffuse texture layer index to the streaming buffer
        diffuse_texture_idxs.push_back(component->graphics_state.diffuse_texture.layer_idx);
      
        Mat4<float> model{};
        model = model.translate(component->graphics_state.position);
        model = model.scale(component->graphics_state.scale);
        buffer.push_back(model.transpose());
      
        log_gl_error();
      }
      glBindBuffer(GL_ARRAY_BUFFER, batch.gl_models_buffer_object);
      glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(Mat4<float>), buffer.data(), GL_DYNAMIC_DRAW);
    
      glBindBuffer(GL_ARRAY_BUFFER, batch.gl_diffuse_texture_layer_idx);
      glBufferData(GL_ARRAY_BUFFER, diffuse_texture_idxs.size() * sizeof(uint32_t), diffuse_texture_idxs.data(),
                   GL_DYNAMIC_DRAW);
    
      glDrawElementsInstanced(GL_TRIANGLES, batch.mesh.indices.size(), GL_UNSIGNED_INT, nullptr, buffer.size());
  
      /// Update render stats
      state.entities += buffer.size();
    }
  }
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
  // TODO: Update all shader programs projection matrices to the new one
  for (auto &batch : graphics_batches) {
    glUseProgram(batch.shader.gl_program);
    GLint projection = glGetUniformLocation(batch.shader.gl_program, "projection");
    glUniformMatrix4fv(projection, 1, GL_FALSE, projection_matrix.data());
  }
}

void Renderer::link_batch(GraphicsBatch& batch) {
  auto vertices = batch.mesh.to_floats();
  
  /// Depth pass setup
  {
    auto program = depth_shader->gl_program;
    glGenVertexArrays(1, &batch.gl_depth_vao);
    glBindVertexArray(batch.gl_depth_vao);
    batch.gl_depth_camera_view = glGetUniformLocation(program, "camera_view");
    
    GLuint depth_vbo;
    glGenBuffers(1, &depth_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, depth_vbo);
    glBufferData(GL_ARRAY_BUFFER, batch.mesh.byte_size_of_vertices(), vertices.data(), GL_DYNAMIC_DRAW);
    
    auto position_attrib = glGetAttribLocation(program, "position");
    glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, position));
    glEnableVertexAttribArray(position_attrib);
    
    auto normal_attrib = glGetAttribLocation(program, "normal");
    glVertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, normal));
    glEnableVertexAttribArray(normal_attrib);
    
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
  
  /// SSAO setup
  {
    auto program = ssao_shader->gl_program;
    glGenVertexArrays(1, &batch.gl_ssao_vao);
    glBindVertexArray(batch.gl_ssao_vao);
    
    GLuint ssao_vbo;
    glGenBuffers(1, &ssao_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ssao_vbo);
    glBufferData(GL_ARRAY_BUFFER, batch.mesh.byte_size_of_vertices(), vertices.data(), GL_DYNAMIC_DRAW);
  
    auto position_attrib = glGetAttribLocation(program, "position");
    glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, position));
    glEnableVertexAttribArray(position_attrib);
  
    // Buffer for all the model matrices
    glGenBuffers(1, &batch.gl_ssao_models_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_ssao_models_buffer_object);
  
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
  
  /// Rendering pass setup
  {
    auto program = batch.shader.gl_program;
    glGenVertexArrays(1, &batch.gl_VAO);
    glBindVertexArray(batch.gl_VAO);
    batch.gl_camera_view = glGetUniformLocation(program, "camera_view");
    batch.gl_camera_position = glGetUniformLocation(program, "camera_position");
    
    GLuint gl_VBO;
    glGenBuffers(1, &gl_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, gl_VBO);
    glBufferData(GL_ARRAY_BUFFER, batch.mesh.byte_size_of_vertices(), vertices.data(), GL_DYNAMIC_DRAW);
    
    // Then set our vertex attributes pointers, only doable AFTER linking
    auto position_attrib = glGetAttribLocation(program, "position");
    glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, position));
    glEnableVertexAttribArray(position_attrib);
    
    auto tex_attrib = glGetAttribLocation(program, "texcoord");
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *) offsetof(Vertex<float>, texCoord));
    glEnableVertexAttribArray(tex_attrib);
    
    // Lights UBO
    auto block_index = glGetUniformBlockIndex(program, "lights_block");
    glBindBuffer(GL_UNIFORM_BUFFER, gl_light_uniform_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, block_index, gl_light_uniform_buffer);
    
    // Buffer for all the model matrices
    glGenBuffers(1, &batch.gl_models_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_models_buffer_object);
    
    auto model_attrib = glGetAttribLocation(program, "model");
    for (int i = 0; i < 4; i++) {
      glVertexAttribPointer(model_attrib + i, 4, GL_FLOAT, GL_FALSE, sizeof(Mat4<float>), (const void *) (sizeof(float) * i * 4));
      glEnableVertexAttribArray(model_attrib + i);
      glVertexAttribDivisor(model_attrib + i, 1);
    }
    
    auto diffuse_texture_idx = glGetAttribLocation(program, "diffuse_texture_idx");
    glGenBuffers(1, &batch.gl_diffuse_texture_layer_idx);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_diffuse_texture_layer_idx);
    glVertexAttribIPointer(diffuse_texture_idx, 1, GL_UNSIGNED_INT, sizeof(uint32_t), 0);
    glEnableVertexAttribArray(diffuse_texture_idx);
    glVertexAttribDivisor(diffuse_texture_idx, 1);
    
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, batch.mesh.byte_size_of_indices(), batch.mesh.indices.data(), GL_STATIC_DRAW);
    
    glUseProgram(program); // Must use the program object before accessing uniforms!
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection_matrix.data());
  }
}

uint64_t Renderer::add_to_batch(RenderComponent* comp, Shader shader) {
  auto mesh_id = comp->graphics_state.mesh_id;
  auto& g_state = comp->graphics_state;
  for (auto& batch : graphics_batches) {
    if (batch.mesh_id == mesh_id && batch.shader == shader) {
      batch.components.push_back(comp);
      // TODO: Check if batch buffer is in use and initialized
      for (const auto& id : batch.texture_ids) {
        if (id == g_state.diffuse_texture.id) {
          g_state.diffuse_texture.layer_idx = batch.layer_idxs[id];
          return batch.id;
        }
      }
      /// Expand texture buffer if needed
      if (batch.diffuse_textures_count + 1 > batch.diffuse_textures_capacity) {
        batch.expand_texture_buffer(&batch.gl_diffuse_texture_array, batch.gl_diffuse_texture_type);
      }
  
      /// Load all the GState's textures
      RawTexture texture = batch.load_textures(&comp->graphics_state);
      texture_manager.insert(texture);
      
      /// Assign layer index to the latest the texture and increment
      g_state.diffuse_texture.layer_idx = batch.diffuse_textures_count++;
      /// Add to all the known texture ids in the batch
      batch.texture_ids.push_back(g_state.diffuse_texture.id);
      /// Update the mapping from texuture id to layer idx
      batch.layer_idxs[g_state.diffuse_texture.id] = g_state.diffuse_texture.layer_idx;
      
      /// Upload the texture to OpenGL
      glActiveTexture(batch.gl_diffuse_texture_unit);
      glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, batch.gl_diffuse_texture_array);
      glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,
                      0,                     // Mipmap number
                      0, 0, g_state.diffuse_texture.layer_idx * 6, // xoffset, yoffset, zoffset = layer face
                      512, 512, 6,           // width, height, depth = faces
                      GL_RGB,                // format
                      GL_UNSIGNED_BYTE,      // type
                      texture.data);         // pointer to data
      return batch.id;
    }
  }
  
  GraphicsBatch batch{mesh_id};
  if (comp->graphics_state.diffuse_texture.used) {
    auto buffer_size = 3; // # textures to hold
    batch.init_buffer(&batch.gl_diffuse_texture_array, g_state.diffuse_texture.gl_texture_type, buffer_size);
    batch.diffuse_textures_used = true;
  }
  batch.gl_diffuse_texture_type = comp->graphics_state.diffuse_texture.gl_texture_type;
  batch.mesh = mesh_manager->mesh_from_id(mesh_id);
  batch.shader = shader;
  link_batch(batch);
  
  /// Assign layer index to the latest the texture and increment
  g_state.diffuse_texture.layer_idx = batch.diffuse_textures_count++;
  /// Add to all the known texture ids in the batch
  batch.texture_ids.push_back(g_state.diffuse_texture.id);
  /// Update the mapping from texture id to layer idx
  batch.layer_idxs[g_state.diffuse_texture.id] = g_state.diffuse_texture.layer_idx;
  
  /// Load all the GState's textures
  RawTexture texture = batch.load_textures(&comp->graphics_state);
  texture_manager.insert(texture);
  /// Upload the texture to OpenGL
  glActiveTexture(batch.gl_diffuse_texture_unit);
  glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, batch.gl_diffuse_texture_array);
  glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,
                  0,                     // Mipmap number
                  0, 0, g_state.diffuse_texture.layer_idx * 6, // xoffset, yoffset, zoffset = layer face
                  512, 512, 6,           // width, height, depth = faces
                  GL_RGB,                // format
                  GL_UNSIGNED_BYTE,      // type
                  texture.data);         // pointer to data

  batch.components.push_back(comp);
  graphics_batches.push_back(batch);
  batch.id = graphics_batches.size(); // TODO: Return real ID
  
  return batch.id;
}