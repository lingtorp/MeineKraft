#include "renderer.hpp"

#include <array>

#ifdef WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif

#include "../nodes/model.hpp"
#include "../nodes/entity.hpp"
#include "../util/filesystem.hpp"

#include "camera.hpp"
#include "debug_opengl.hpp"
#include "graphicsbatch.hpp"
#include "meshmanager.hpp"
#include "rendercomponent.hpp"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/// Draw struct taken from OpenGL (see: glMultiDrawElementsIndirect)
struct DrawElementsIndirectCommand {
  uint32_t count = 0;         // # elements (i.e indices)
  uint32_t instanceCount = 0; // # instances (kind of drawcalls)
  uint32_t firstIndex = 0;    // index of the first element in the EBO
  uint32_t baseVertex = 0;    // indices[i] + baseVertex
  uint32_t baseInstance = 0;  // instance = [gl_InstanceID / divisor] + baseInstance
  uint32_t padding0 = 0;      // Padding due to GLSL layout std140 16B alignment rule
  uint32_t padding1 = 0;
  uint32_t padding2 = 0;
};

// FIXME: Remake this to serve a better purpose (unique per line, file like the log_once)
uint32_t Renderer::get_next_free_texture_unit(bool peek) {
  int32_t max_texture_units;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
  static int32_t next_texture_unit = -1;
  if (peek) {
    return next_texture_unit + 1;
  }
  next_texture_unit++;
  if (next_texture_unit >= max_texture_units) {
    Log::error("Reached max texture units: " + std::to_string(max_texture_units));
    exit(1);
  }
  return next_texture_unit;
}

// FIXME: Remake this to serve a better purpose (unique per line, file like the log_once)
uint32_t Renderer::get_next_free_image_unit(bool peek) {
  int32_t max_image_units;
  glGetIntegerv(GL_MAX_IMAGE_UNITS, &max_image_units);
  static int32_t next_image_unit = -1;
  if (peek) {
    return next_image_unit + 1;
  }
  next_image_unit++;
  if (next_image_unit >= max_image_units) {
    Log::error("Reached max image units: " + std::to_string(max_image_units));
    exit(1);
  }
  return next_image_unit;
}

void Renderer::load_environment_map(const std::vector<std::string>& faces) {
  Texture texture;
  const auto resource = TextureResource{faces};
  texture.data = Texture::load_textures(resource);
  if (texture.data.pixels) {
    texture.gl_texture_target = GL_TEXTURE_CUBE_MAP_ARRAY;
    texture.id = resource.to_hash();

    gl_environment_map_texture_unit = Renderer::get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_environment_map_texture_unit);
    uint32_t gl_environment_map_texture = 0;
    glGenTextures(1, &gl_environment_map_texture);
    glBindTexture(texture.gl_texture_target, gl_environment_map_texture);
    glTexParameteri(texture.gl_texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(texture.gl_texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexStorage3D(texture.gl_texture_target, 1, GL_RGB8, texture.data.width, texture.data.height, texture.data.faces);
    glTexSubImage3D(texture.gl_texture_target, 0, 0, 0, 0, texture.data.width, texture.data.height, texture.data.faces, GL_RGB, GL_UNSIGNED_BYTE, texture.data.pixels);
    environment_map = texture;

    glObjectLabel(GL_TEXTURE, gl_environment_map_texture, -1, "Environment texture");
  } else {
    Log::warn("Could not load environment map");
  }
}

// NOTE: AABB passed is assumed to be the Scene AABB
static glm::mat4 shadowmap_transform(const AABB& aabb, const DirectionalLight& light) {
  const float diameter = aabb.max_axis();
  const float left   = -diameter / 2.0f;
  const float right  =  diameter / 2.0f;
  const float bottom = -diameter / 2.0f; 
  const float top    =  diameter / 2.0f;
  const float znear  = 0.0f;
  const float zfar   = diameter;

  const glm::vec3 center = aabb.center().as_glm();
  const glm::vec3 light_direction = light.direction.normalize().as_glm();
  const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::mat4 light_view_transform = glm::lookAt(center - (diameter / 2.0f) * light_direction, center, up);

  return glm::ortho(left, right, bottom, top, znear, zfar) * light_view_transform;
}

// NOTE: AABB passed is assumed to be the Scene AABB
static glm::mat4 orthographic_projection(const AABB& aabb) {
  const float voxel_grid_dimension = aabb.max_axis();
	const float left   = -voxel_grid_dimension;
	const float right  =  voxel_grid_dimension;
	const float bottom = -voxel_grid_dimension;
	const float top    =  voxel_grid_dimension;
	const float znear  =  0.0f;
	const float zfar   =  2.0f * voxel_grid_dimension;
	const glm::mat4 ortho  = glm::ortho(left, right, bottom, top, znear, zfar);
  const glm::vec3 center = aabb.center().as_glm();
  const glm::vec3 offset = glm::vec3(0.0f, 0.0f, voxel_grid_dimension);
  return ortho * glm::lookAt(center - offset, center, glm::vec3(0.0f, 1.0f, 0.0f));
}

/// Returns the planes from the frustrum matrix in order; {left, right, bottom, top, near, far} (MV = world space with normal poiting to positive halfspace)
/// See: http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
std::array<glm::vec4, 6> extract_planes(const glm::mat4& mat) {
  const auto left_plane = glm::vec4(mat[3][0] + mat[0][0],
    mat[3][1] + mat[0][1],
    mat[3][2] + mat[0][2],
    mat[3][3] + mat[0][3]);

  const auto right_plane = glm::vec4(mat[3][0] - mat[0][0],
    mat[3][1] - mat[0][1],
    mat[3][2] - mat[0][2],
    mat[3][3] - mat[0][3]);

  const auto bot_plane = glm::vec4(mat[3][0] + mat[1][0],
    mat[3][1] + mat[1][1],
    mat[3][2] + mat[1][2],
    mat[3][3] + mat[1][3]);

  const auto top_plane = glm::vec4(mat[3][0] - mat[1][0],
    mat[3][1] - mat[1][1],
    mat[3][2] - mat[1][2],
    mat[3][3] - mat[1][3]);

  const auto near_plane = glm::vec4(mat[3][0] + mat[2][0],
    mat[3][1] + mat[2][1],
    mat[3][2] + mat[2][2],
    mat[3][3] + mat[2][3]);

  const auto far_plane = glm::vec4(mat[3][0] - mat[2][0],
    mat[3][1] - mat[2][1],
    mat[3][2] - mat[2][2],
    mat[3][3] - mat[2][3]);
  return { normalize(left_plane), normalize(right_plane), normalize(bot_plane), normalize(top_plane), normalize(near_plane), normalize(far_plane) };
}

/// Generates (cone-direction, cone-weight) list
std::vector<Vec4f> generate_diffuse_cones(const size_t count) {
  assert(count >= 0);

  if (count == 0) {
    return {};
  }

  std::vector<Vec4f> cones(count);

  if (count == 1) {
    cones[0] = Vec4f(0.0f, 1.0f, 0.0f, 1.0f);
    return cones;
  }

  const float rad_delta = 2.0f * M_PI / (count - 1.0f);
  const float theta = glm::radians(45.0f);

  // Normal cone
  cones[0] = Vec4f(0.0f, 1.0f, 0.0f, 1.0f / (count - 1.0f));
  for (size_t i = 0; i < count - 1; i++) {
    const Vec3f direction = Vec3f(std::sin(theta) * std::cos(i * rad_delta),
                                  std::cos(theta),
                                  std::sin(theta) * std::sin(i * rad_delta));
    cones[i + 1] = Vec4f(direction, (1.0f - cones[0].w) / (count - 1.0f));
  }

  return cones;
}

// Center perserved generation of scaled AABBs of the Scene AABB
std::vector<AABB> generate_clipmaps_from_scene_aabb(const AABB& scene,
                                                    const size_t num_clipmaps) {
  assert(num_clipmaps > 1);

  std::vector<AABB> clipmaps(num_clipmaps);
  clipmaps[num_clipmaps - 1] = scene;

  for (size_t i = 0; i < num_clipmaps - 1; i++) {
    const float scaling_factor = 1.0f / (std::pow(2.0f, (num_clipmaps - i - 1)));
    clipmaps[i].max = (scene.max - scene.center()) * scaling_factor + scene.center();
    clipmaps[i].min = (scene.min - scene.center()) * scaling_factor + scene.center();
  }

  return clipmaps;
}

Renderer::~Renderer() = default;

Renderer::Renderer(const Resolution& screen): screen(screen), graphics_batches{} {
  /// Global geometry pass framebuffer
  glGenFramebuffers(1, &gl_depth_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
  glObjectLabel(GL_FRAMEBUFFER, gl_depth_fbo, -1, "GBuffer FBO");

  // Global depth buffer
  gl_depth_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_depth_texture_unit);
  glGenTextures(1, &gl_depth_texture);
  glBindTexture(GL_TEXTURE_2D, gl_depth_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  // glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT); // Default value (intention only to read depth values from texture)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, screen.width, screen.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gl_depth_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_depth_texture, -1, "GBuffer depth texture");

  // Global normal buffer
  gl_geometric_normal_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_geometric_normal_texture_unit);
  glGenTextures(1, &gl_geometric_normal_texture);
  glBindTexture(GL_TEXTURE_2D, gl_geometric_normal_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_geometric_normal_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_geometric_normal_texture, -1, "GBuffer geometric normal texture");

  // Global position buffer
  gl_position_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_position_texture_unit);
  glGenTextures(1, &gl_position_texture);
  glBindTexture(GL_TEXTURE_2D, gl_position_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gl_position_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_position_texture, -1, "GBuffer position texture");

  // Global diffuse buffer
  gl_diffuse_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_diffuse_texture_unit);
  glGenTextures(1, &gl_diffuse_texture);
  glBindTexture(GL_TEXTURE_2D, gl_diffuse_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gl_diffuse_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_diffuse_texture, -1, "GBuffer diffuse texture");

  // Global PBR parameters buffer
  gl_pbr_parameters_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_pbr_parameters_texture_unit);
  glGenTextures(1, &gl_pbr_parameters_texture);
  glBindTexture(GL_TEXTURE_2D, gl_pbr_parameters_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, gl_pbr_parameters_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_pbr_parameters_texture, -1, "GBuffer PBR parameters texture");

  // Global emissive map
  gl_emissive_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_emissive_texture_unit);
  glGenTextures(1, &gl_emissive_texture);
  glBindTexture(GL_TEXTURE_2D, gl_emissive_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, gl_emissive_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_emissive_texture, -1, "GBuffer emissive texture");

  // Global shading model id
  gl_shading_model_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_shading_model_texture_unit);
  glGenTextures(1, &gl_shading_model_texture);
  glBindTexture(GL_TEXTURE_2D, gl_shading_model_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, screen.width, screen.height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, gl_shading_model_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_shading_model_texture, -1, "GBuffer shading ID texture");

  // Global tangent space normal map
  gl_tangent_normal_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_tangent_normal_texture_unit); 
  glGenTextures(1, &gl_tangent_normal_texture);
  glBindTexture(GL_TEXTURE_2D, gl_tangent_normal_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, gl_tangent_normal_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_tangent_normal_texture, -1, "GBuffer tangent normal texture");

  // Global tangent map
  gl_tangent_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_tangent_texture_unit);
  glGenTextures(1, &gl_tangent_texture);
  glBindTexture(GL_TEXTURE_2D, gl_tangent_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT7, gl_tangent_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_tangent_texture, -1, "GBuffer tangent texture");

  uint32_t depth_attachments[8] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
  glDrawBuffers(8, depth_attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("Lightning framebuffer status not complete.");
  }

  /// Point lightning framebuffer
  glGenFramebuffers(1, &gl_lightning_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_lightning_fbo);

  GLuint gl_lightning_rbo;
  glGenRenderbuffers(1, &gl_lightning_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, gl_lightning_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screen.width, screen.height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_lightning_rbo);

  gl_lightning_texture_unit = Renderer::get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_lightning_texture_unit);
  glGenTextures(1, &gl_lightning_texture);
  glBindTexture(GL_TEXTURE_2D, gl_lightning_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_lightning_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_lightning_texture, -1, "Pointlighting texture");

  uint32_t lightning_attachments[1] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(1, lightning_attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("Point lightning framebuffer status not complete.");
  }

  /// Lightning pass shader
  bool success = false;
  std::string err_msg;
  lightning_shader = new Shader{Filesystem::base + "shaders/lightning.vert", Filesystem::base + "shaders/lightning.frag" };
  std::tie(success, err_msg) = lightning_shader->compile();
  if (!success) {
    Log::error("Lightning shader compilation failed; " + err_msg);
  }

  /// Point light pass setup
  {
    const auto program = lightning_shader->gl_program;
    glGenVertexArrays(1, &gl_lightning_vao);
    glBindVertexArray(gl_lightning_vao);

    GLuint gl_vbo;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  }

  /// Create SSBO for the PointLights
  pointlights.emplace_back(PointLight(Vec3f(200.0f)));
  glGenBuffers(1, &gl_pointlights_ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl_pointlights_ssbo);
  const auto flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
  glBufferStorage(GL_SHADER_STORAGE_BUFFER, pointlights.size() * sizeof(PointLight), nullptr, flags);
  gl_pointlights_ssbo_ptr = (uint8_t*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, pointlights.size() * sizeof(PointLight), flags);

  const uint32_t gl_pointlight_ssbo_binding_point_idx = 4; // Default value in lightning.frag
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_pointlight_ssbo_binding_point_idx, gl_pointlights_ssbo);
  std::memcpy(gl_pointlights_ssbo_ptr, pointlights.data(), pointlights.size() * sizeof(PointLight));

  // View frustum culling shader
  cull_shader = new ComputeShader{Filesystem::base + "shaders/culling.comp.glsl"};

  // Directional shadow mapping setup
  {
    glGenFramebuffers(1, &gl_shadowmapping_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_shadowmapping_fbo);
		glObjectLabel(GL_FRAMEBUFFER, gl_shadowmapping_fbo, -1, "Shadowmap FBO");
    gl_shadowmapping_texture_unit = Renderer::get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_shadowmapping_texture_unit);
    glGenTextures(1, &gl_shadowmapping_texture);
    glBindTexture(GL_TEXTURE_2D, gl_shadowmapping_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, SHADOWMAP_W, SHADOWMAP_H);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_shadowmapping_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_shadowmapping_texture, -1, "Shadowmap texture");
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Directional shadow mapping FBO not complete"); exit(1);
    }

    // Shaders
    shadowmapping_shader = new Shader(Filesystem::base + "shaders/shadowmapping.vert", 
                                      Filesystem::base + "shaders/shadowmapping.frag");
    std::tie(success, err_msg) = shadowmapping_shader->compile();
    if (!success) {
      Log::error("Could not compile directional shadow mapping shaders"); exit(1);
    }
  }

  /// Voxelization pass
  {
    voxelization_shader = new Shader(Filesystem::base + "shaders/voxelization.vert",
                                     Filesystem::base + "shaders/voxelization.geom",
                                     Filesystem::base + "shaders/voxelization.frag");

    if (!voxelization_shader->compiled_successfully) {
      Log::error("Voxelization shader did not compile successfully"); exit(-1);
    }

    glGenFramebuffers(1, &gl_voxelization_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_voxelization_fbo);
		glObjectLabel(GL_FRAMEBUFFER, gl_voxelization_fbo, -1, "Voxelization FBO");

    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      // Global radiance voxel buffer
      gl_voxel_radiance_image_units[i] = get_next_free_image_unit();
      gl_voxel_radiance_texture_units[i] = get_next_free_texture_unit();
      glActiveTexture(GL_TEXTURE0 + gl_voxel_radiance_texture_units[i]);
      glGenTextures(1, &gl_voxel_radiance_textures[i]);
      glBindTexture(GL_TEXTURE_3D, gl_voxel_radiance_textures[i]);
      glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, clipmaps.size[i], clipmaps.size[i], clipmaps.size[i]);
      glBindImageTexture(gl_voxel_radiance_image_units[i], gl_voxel_radiance_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      const std::string radiance_object_label = "Clipmap #" + std::to_string(i) + " radiance texture";
      glObjectLabel(GL_TEXTURE, gl_voxel_radiance_textures[i], -1, radiance_object_label.c_str());
 
      // Global opacity voxel buffer
      gl_voxel_opacity_image_units[i] = get_next_free_image_unit();
      gl_voxel_opacity_texture_units[i] = get_next_free_texture_unit();
      glActiveTexture(GL_TEXTURE0 + gl_voxel_opacity_texture_units[i]);
      glGenTextures(1, &gl_voxel_opacity_textures[i]);
      glBindTexture(GL_TEXTURE_3D, gl_voxel_opacity_textures[i]);
      glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, clipmaps.size[i], clipmaps.size[i], clipmaps.size[i]);
      glBindImageTexture(gl_voxel_opacity_image_units[i], gl_voxel_opacity_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      const std::string opacity_object_label = "Clipmap #" + std::to_string(i) + " opacity texture";
      glObjectLabel(GL_TEXTURE, gl_voxel_opacity_textures[i], -1, opacity_object_label.c_str());
    }

    // Reuse shadowmap depth attachment for FBO completeness (disabled anyway)
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_shadowmapping_texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Voxelization FBO not complete"); exit(1);
    }

    /// Opacity normalization compute shader subpass
    voxelization_opacity_norm_shader = new ComputeShader(Filesystem::base + "shaders/voxelization-opacity-normalization.comp");
  }

  /// Voxel visualization pass
  if (voxel_visualization_enabled) {
    voxel_visualization_shader = new Shader(Filesystem::base + "shaders/voxel-visualization.vert",
                                            Filesystem::base + "shaders/voxel-visualization.geom",
                                            Filesystem::base + "shaders/voxel-visualization.frag");

    if (!voxel_visualization_shader->compiled_successfully) {
      Log::error("Failed to compile voxel visualization shaders"); exit(-1);
    } 

    const auto program = voxel_visualization_shader->gl_program;
    glUseProgram(program);

    glGenFramebuffers(1, &gl_voxel_visualization_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_voxel_visualization_fbo);
    glObjectLabel(GL_FRAMEBUFFER, gl_voxel_visualization_fbo, -1, "Voxel visualization FBO");

    glActiveTexture(GL_TEXTURE0 + gl_lightning_texture_unit);
    glGenTextures(1, &gl_voxel_visualization_texture);
    glBindTexture(GL_TEXTURE_2D, gl_voxel_visualization_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_voxel_visualization_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_voxel_visualization_texture, -1, "Voxel visualization texture");

    uint32_t fbo_attachments[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, fbo_attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Voxel visualization FBO not complete"); exit(1);
    }

    glGenVertexArrays(1, &gl_voxel_visualization_vao);
    glBindVertexArray(gl_voxel_visualization_vao);

    GLuint gl_vbo = 0;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    const Vec3f vertex(1.0f, 1.0f, 1.0f);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3f), &vertex, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  }

  /// Voxel cone tracing pass
  {
    /// Create SSBO for the diffuse cones
    const size_t MAX_CONES = 12; 
    const std::vector<Vec4f> cones = generate_diffuse_cones(state.num_diffuse_cones);

    glGenBuffers(1, &gl_vct_diffuse_cones_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl_vct_diffuse_cones_ssbo);
    const auto flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, MAX_CONES * sizeof(Vec4f), nullptr, flags);
    gl_vct_diffuse_cones_ssbo_ptr = (uint8_t*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, MAX_CONES * sizeof(Vec4f), flags);

    const uint32_t gl_diffuse_cones_ssbo_binding_point_idx = 8; // Default value in voxel-cone-tracing.frag
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_diffuse_cones_ssbo_binding_point_idx, gl_vct_diffuse_cones_ssbo);
    std::memcpy(gl_vct_diffuse_cones_ssbo_ptr, cones.data(), cones.size() * sizeof(Vec4f));

    vct_shader = new Shader(Filesystem::base + "shaders/voxel-cone-tracing.vert", 
                            Filesystem::base + "shaders/voxel-cone-tracing.frag");
    std::tie(success, err_msg) = vct_shader->compile();
    if (!success) {
      Log::error("Could not compile voxel-cone-tracing shaders"); exit(-1);
    }

    const auto program = vct_shader->gl_program;
    glUseProgram(program);

    glGenFramebuffers(1, &gl_vct_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_vct_fbo);

		glActiveTexture(GL_TEXTURE0 + gl_lightning_texture_unit);
		glGenTextures(1, &gl_vct_texture);
		glBindTexture(GL_TEXTURE_2D, gl_vct_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_vct_texture, 0);
		glObjectLabel(GL_TEXTURE, gl_vct_texture, -1, "VCT lighting texture");

    uint32_t fbo_attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, fbo_attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("VCT fbo not complete"); exit(-1); 
    }

    glGenVertexArrays(1, &gl_vct_vao);
    glBindVertexArray(gl_vct_vao);

    GLuint gl_vbo = 0;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  }

  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
}

bool Renderer::init() {
  std::vector<AABB> aabbs = generate_clipmaps_from_scene_aabb(scene->aabb, NUM_CLIPMAPS);
  for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
    clipmaps.aabb[i] = aabbs[i];
    Log::info("--------------------");
    Log::info(aabbs[i]);
    Log::info(aabbs[i].center());
    Log::info(aabbs[i].max_axis());
    Log::info(aabbs[i].max_axis() / clipmaps.size[i]);
  }
  return true;
}

void Renderer::render(const uint32_t delta) {
  state = RenderState(state);
  state.frame++;

  const glm::mat4 camera_transform = projection_matrix * scene->camera->transform(); // TODO: Camera handling needs to be reworked

  /// Renderer caches the transforms of components thus we need to fetch the ones who changed during the last frame
  if (state.frame % 10 == 0) {
    TransformSystem::instance().reset_dirty();
  }
  update_transforms();

  static GLsync syncs[3] = {nullptr, nullptr, nullptr};

  if (syncs[state.frame % 3]) {
    while (true) {
      GLenum wait_result = glClientWaitSync(syncs[state.frame % 3], GL_SYNC_FLUSH_COMMANDS_BIT, 1);
      if (wait_result == GL_CONDITION_SATISFIED || wait_result == GL_ALREADY_SIGNALED) { break; }
    }
  }

  // Update all ptr indices in all of the batches
  for (size_t i = 0; i < graphics_batches.size(); i++) {
    auto& batch = graphics_batches[i];
    batch.gl_curr_ibo_idx = state.frame % batch.gl_ibo_count;
  }

  // Reset the draw commands
  for (size_t i = 0; i < graphics_batches.size(); i++) {
    const auto& batch = graphics_batches[i];
    ((DrawElementsIndirectCommand*)batch.gl_ibo_ptr)[batch.gl_curr_ibo_idx].instanceCount = 0;
  }

  // Update pointlights
  std::memcpy(gl_pointlights_ssbo_ptr, pointlights.data(), pointlights.size() * sizeof(PointLight));

  {
    pass_started("Culling pass");

    // NOTE: Extraction of frustum planes are performed on the transpose (because of column/row-major difference).
    // FIXME: Use the Direct3D way of extraction instead since GLM appears to store the matrix in a row-major way.
    const glm::mat4 proj_view = projection_matrix * camera_transform;
    const std::array<glm::vec4, 6> frustum = extract_planes(glm::transpose(proj_view));

    glUseProgram(cull_shader->gl_program);
    glUniform4fv(glGetUniformLocation(cull_shader->gl_program, "frustum_planes"), 6, glm::value_ptr(frustum[0]));
    for (size_t i = 0; i < graphics_batches.size(); i++) {
      const auto& batch = graphics_batches[i];

      // Rebind the draw cmd SSBO for the compute shader to the current batch
      const uint32_t gl_draw_cmd_binding_point = 0; // Defaults to 0 in the culling compute shader
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_draw_cmd_binding_point, batch.gl_ibo);

      const uint32_t gl_instance_idx_binding_point = 1; // Defaults to 1 in the culling compute shader
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_instance_idx_binding_point, batch.gl_instance_idx_buffer);

      const uint32_t gl_bounding_volume_binding_point = 5; // Defaults to 5 in the culling compute shader
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_bounding_volume_binding_point, batch.gl_bounding_volume_buffer);

      glUniform1ui(glGetUniformLocation(cull_shader->gl_program, "NUM_INDICES"), batch.mesh.indices.size());
      glUniform1ui(glGetUniformLocation(cull_shader->gl_program, "DRAW_CMD_IDX"), batch.gl_curr_ibo_idx);

      glDispatchCompute(batch.objects.transforms.size(), 1, 1);
    }
    // TODO: Is this barrier required?
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT); // Buffer objects affected by this bit are derived from the GL_DRAW_INDIRECT_BUFFER binding.

    pass_ended();
  }

  pass_started("Directional shadow mapping pass");
  const glm::mat4 light_space_transform = shadowmap_transform(scene->aabb, directional_light);
  {
    glCullFace(GL_FRONT);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_shadowmapping_fbo);
    glViewport(0, 0, SHADOWMAP_W, SHADOWMAP_H);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT); // Always update the depth buffer with the new values
    const uint32_t program = shadowmapping_shader->gl_program;
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "uLight_space_transform"), 1, GL_FALSE, glm::value_ptr(light_space_transform));
    for (size_t i = 0; i < graphics_batches.size(); i++) {
      const auto& batch = graphics_batches[i];
      glBindVertexArray(batch.gl_shadowmapping_vao);
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.gl_ibo); // GL_DRAW_INDIRECT_BUFFER is global context state

      const uint32_t gl_models_binding_point = 2; // Defaults to 2 in geometry.vert shader
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_models_binding_point, batch.gl_depth_model_buffer);

      const uint64_t draw_cmd_offset = batch.gl_curr_ibo_idx * sizeof(DrawElementsIndirectCommand);
      glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*) draw_cmd_offset, 1, sizeof(DrawElementsIndirectCommand));
    }
    glViewport(0, 0, screen.width, screen.height);
    glCullFace(GL_BACK);

    pass_ended();
  }

  {
    pass_started("Geometry pass");

    glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Always update the depth buffer with the new values
    for (size_t i = 0; i < graphics_batches.size(); i++) {
      const auto& batch = graphics_batches[i];
      const auto program = batch.depth_shader.gl_program;
      glUseProgram(program);
      glBindVertexArray(batch.gl_depth_vao);
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.gl_ibo); // GL_DRAW_INDIRECT_BUFFER is global context state

			glUniformMatrix4fv(glGetUniformLocation(program, "camera_view"), 1, GL_FALSE, glm::value_ptr(camera_transform));

      const uint32_t gl_models_binding_point = 2; // Defaults to 2 in geometry.vert shader
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_models_binding_point, batch.gl_depth_model_buffer);

      const uint32_t gl_material_binding_point = 3; // Defaults to 3 in geometry.frag shader
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_material_binding_point, batch.gl_material_buffer);

      glActiveTexture(GL_TEXTURE0 + batch.gl_diffuse_texture_unit);
      glBindTexture(GL_TEXTURE_2D_ARRAY, batch.gl_diffuse_texture_array);

      if (batch.gl_metallic_roughness_texture != 0) {
        glActiveTexture(GL_TEXTURE0 + batch.gl_metallic_roughness_texture_unit);
        glBindTexture(GL_TEXTURE_2D, batch.gl_metallic_roughness_texture);
      }

      if (batch.gl_tangent_normal_texture != 0) {
        glActiveTexture(GL_TEXTURE0 + batch.gl_tangent_normal_texture_unit);
        glBindTexture(GL_TEXTURE_2D, batch.gl_tangent_normal_texture);
      }

      if (batch.gl_emissive_texture != 0) {
        glActiveTexture(GL_TEXTURE0 + batch.gl_emissive_texture_unit);
        glBindTexture(GL_TEXTURE_2D, batch.gl_emissive_texture);
      }

      const uint64_t draw_cmd_offset = batch.gl_curr_ibo_idx * sizeof(DrawElementsIndirectCommand);
      glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*) draw_cmd_offset, 1, sizeof(DrawElementsIndirectCommand));
    }

    pass_ended();
  }

  if (state.voxelize) {
    if (!state.always_voxelize) {
      state.voxelize = false;
    }

    pass_started("Voxelization pass");

    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      glClearTexImage(gl_voxel_radiance_textures[i], 0, GL_RGBA, GL_FLOAT, nullptr); 
      glClearTexImage(gl_voxel_opacity_textures[i], 0, GL_RGBA, GL_FLOAT, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, gl_voxelization_fbo);

		const auto program = voxelization_shader->gl_program;
		glUseProgram(program);

		// Orthogonal projection along +z-axis
		glm::mat4 orthos[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      orthos[i] = orthographic_projection(clipmaps.aabb[i]);
    }
		glUniformMatrix4fv(glGetUniformLocation(program, "uOrthos"), NUM_CLIPMAPS, GL_FALSE, glm::value_ptr(orthos[0]));

    float scaling_factors[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      scaling_factors[i] = 1.0f / clipmaps.aabb[i].max_axis();
    }
    glUniform1fv(glGetUniformLocation(program, "uScaling_factors"), NUM_CLIPMAPS, scaling_factors);

    Vec3f aabb_centers[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      aabb_centers[i] = clipmaps.aabb[i].center();
    }
    glUniform3fv(glGetUniformLocation(program, "uAABB_centers"), NUM_CLIPMAPS, &aabb_centers[0].x);

    // Shadowmapping
    glUniform1f(glGetUniformLocation(program, "uShadow_bias"), state.shadow_bias);
    glUniform3fv(glGetUniformLocation(program, "uDirectional_light_direction"), 1, &directional_light.direction.x);
    glUniformMatrix4fv(glGetUniformLocation(program, "uLight_space_transform"), 1, GL_FALSE, glm::value_ptr(light_space_transform));
    glUniform1i(glGetUniformLocation(program, "uShadowmap"), gl_shadowmapping_texture_unit);
    glUniform1iv(glGetUniformLocation(program, "uClipmap_sizes"), NUM_CLIPMAPS, clipmaps.size);
		glUniform1iv(glGetUniformLocation(program, "uVoxelRadiance"), NUM_CLIPMAPS, gl_voxel_radiance_image_units);
    glUniform1iv(glGetUniformLocation(program, "uVoxelOpacity"), NUM_CLIPMAPS, gl_voxel_opacity_image_units);
    glUniform1i(glGetUniformLocation(program, "uConservative_rasterization_enabled"), state.conservative_rasterization);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    Vec4f viewports[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      viewports[i] = Vec4f(0.0f, 0.0f, clipmaps.size[i], clipmaps.size[i]);
    }
    glViewportArrayv(0, NUM_CLIPMAPS, &viewports[0].x);

    for (size_t i = 0; i < graphics_batches.size(); i++) {
      const auto &batch = graphics_batches[i];
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
 
    pass_ended();

    // NOTE: Opacity normalization subpass is way too slow around 12.7 ms (26% of frame time)
    if constexpr(false) {
      pass_started("Opacity normalization subpass");

      const auto program = voxelization_opacity_norm_shader->gl_program;
      glUseProgram(program);
      for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
        glBindImageTexture(gl_voxel_radiance_image_units[i], gl_voxel_radiance_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
        glUniform1i(glGetUniformLocation(program, "voxels"), gl_voxel_radiance_image_units[i]);
        glDispatchCompute(clipmaps.size[i], clipmaps.size[i], clipmaps.size[i]);
        glBindImageTexture(gl_voxel_radiance_image_units[i], gl_voxel_radiance_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
      }
      
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Due to incoherent mem. access need to sync read and usage of voxel data

      pass_ended();
    }
  }

  if (voxel_visualization_enabled) {
    pass_started("Voxel visualization pass");

    const auto program = voxel_visualization_shader->gl_program;
    glUseProgram(program);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_voxel_visualization_fbo);
    glBindVertexArray(gl_voxel_visualization_vao);

    glActiveTexture(GL_TEXTURE0 + gl_lightning_texture_unit);
    glBindTexture(GL_TEXTURE_2D, gl_voxel_visualization_texture);

    glUniformMatrix4fv(glGetUniformLocation(program, "uCamera_view"), 1, GL_FALSE, glm::value_ptr(camera_transform));
    glUniform1iv(glGetUniformLocation(program, "uVoxelOpacity"), NUM_CLIPMAPS, gl_voxel_opacity_image_units);
    glUniform1iv(glGetUniformLocation(program, "uVoxelRadiance"), NUM_CLIPMAPS, gl_voxel_radiance_image_units);
    glUniform1iv(glGetUniformLocation(program, "uClipmap_sizes"), NUM_CLIPMAPS, clipmaps.size);

    Vec3f clipmap_aabb_mins[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      clipmap_aabb_mins[i] = clipmaps.aabb[i].min;
    }
    glUniform3fv(glGetUniformLocation(program, "uAABB_mins"), NUM_CLIPMAPS, &clipmap_aabb_mins[0].x);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      glUniform1ui(glGetUniformLocation(program, "uClipmapIdx"), i);
      glDrawArrays(GL_POINTS, 0, clipmaps.size[i] * clipmaps.size[i] * clipmaps.size[i]);
    }

    pass_ended();
  }

  {
    pass_started("Voxel cone tracing pass");

    const auto program = vct_shader->gl_program;
		glBindFramebuffer(GL_FRAMEBUFFER, gl_vct_fbo);

    glBindVertexArray(gl_vct_vao);
		glUseProgram(program);

		glActiveTexture(GL_TEXTURE0 + gl_lightning_texture_unit);
		glBindTexture(GL_TEXTURE_2D, gl_vct_texture);  

    // User customizable
    glUniform1i(glGetUniformLocation(program, "uDirect_lighting"), state.direct_lighting);
    glUniform1i(glGetUniformLocation(program, "uIndirect_lighting"), state.indirect_lighting);
    glUniform1i(glGetUniformLocation(program, "uDiffuse_lighting"), state.diffuse_lighting);
    glUniform1i(glGetUniformLocation(program, "uSpecular_lighting"), state.specular_lighting);
    glUniform1f(glGetUniformLocation(program, "uRoughness"), state.roughness);
    const float roughness_aperature = glm::radians(state.roughness_aperature);
    glUniform1f(glGetUniformLocation(program, "uRoughness_aperature"), roughness_aperature);
    glUniform1f(glGetUniformLocation(program, "uMetallic"), state.metallic);
    const float metallic_aperature = glm::radians(state.metallic_aperature);
    glUniform1f(glGetUniformLocation(program, "uMetallic_aperature"), metallic_aperature);

    // Shadowmapping
    glUniform1f(glGetUniformLocation(program, "uShadow_bias"), state.shadow_bias);
    glUniform3fv(glGetUniformLocation(program, "uDirectional_light_direction"), 1, &directional_light.direction.x);
    glUniformMatrix4fv(glGetUniformLocation(program, "uLight_space_transform"), 1, GL_FALSE, glm::value_ptr(light_space_transform));
    glUniform1i(glGetUniformLocation(program, "uShadowmap"), gl_shadowmapping_texture_unit);
    glUniform1ui(glGetUniformLocation(program, "uShadowmap_width"), SHADOWMAP_W);
    glUniform1ui(glGetUniformLocation(program, "uShadowmap_height"), SHADOWMAP_H);

    glUniform3fv(glGetUniformLocation(program, "uCamera_position"), 1, &scene->camera->position.x);

    glUniform1ui(glGetUniformLocation(program, "uNum_diffuse_cones"), state.num_diffuse_cones);
    const std::vector<Vec4f> cones = generate_diffuse_cones(state.num_diffuse_cones);
    std::memcpy(gl_vct_diffuse_cones_ssbo_ptr, cones.data(), cones.size() * sizeof(Vec4f));

    glUniform1i(glGetUniformLocation(program, "uNormalmapping"), state.normalmapping);

    float scaling_factors[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      scaling_factors[i] = 1.0f / clipmaps.aabb[i].max_axis();
    }
    glUniform1fv(glGetUniformLocation(program, "uScaling_factors"), NUM_CLIPMAPS, scaling_factors);

    const float voxel_size_LOD0 = clipmaps.aabb[0].max_axis() / float(clipmaps.size[0]);
    glUniform1f(glGetUniformLocation(program, "uVoxel_size_LOD0"), voxel_size_LOD0);

    Vec3f clipmap_aabb_centers[NUM_CLIPMAPS];
    Vec3f clipmap_aabb_mins[NUM_CLIPMAPS];
    Vec3f clipmap_aabb_maxs[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      clipmap_aabb_centers[i] = clipmaps.aabb[i].center();
      clipmap_aabb_mins[i] = clipmaps.aabb[i].min;
      clipmap_aabb_maxs[i] = clipmaps.aabb[i].max;
    }
    glUniform3fv(glGetUniformLocation(program, "uAABB_centers"), NUM_CLIPMAPS, &clipmap_aabb_centers[0].x);
    glUniform3fv(glGetUniformLocation(program, "uAABB_mins"), NUM_CLIPMAPS, &clipmap_aabb_mins[0].x);
    glUniform3fv(glGetUniformLocation(program, "uAABB_maxs"), NUM_CLIPMAPS, &clipmap_aabb_maxs[0].x);

		glUniform1iv(glGetUniformLocation(program, "uVoxelRadiance"), NUM_CLIPMAPS, gl_voxel_radiance_texture_units);
    glUniform1iv(glGetUniformLocation(program, "uVoxelOpacity"), NUM_CLIPMAPS, gl_voxel_opacity_texture_units);

    glUniform1f(glGetUniformLocation(program, "uScreen_width"), (float)screen.width);
    glUniform1f(glGetUniformLocation(program, "uScreen_height"), (float)screen.height);
    glUniform1i(glGetUniformLocation(program, "uDiffuse"), gl_diffuse_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uPosition"), gl_position_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uNormal"), gl_geometric_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uPBR_parameters"), gl_pbr_parameters_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gl_tangent_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uTangent"), gl_tangent_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uEmissive"), gl_emissive_texture_unit); 

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    pass_ended();
  }

  if (false) {
    pass_started("Lightning pass");

    const auto program = lightning_shader->gl_program;
    glBindFramebuffer(GL_FRAMEBUFFER, gl_lightning_fbo);

    glBindVertexArray(gl_lightning_vao);
    glUseProgram(program);

		glActiveTexture(GL_TEXTURE0 + gl_lightning_texture_unit);
		glBindTexture(GL_TEXTURE_2D, gl_lightning_texture);

    glUniform1i(glGetUniformLocation(program, "environment_map_sampler"), gl_environment_map_texture_unit);
    glUniform1i(glGetUniformLocation(program, "shading_model_id_sampler"), gl_shading_model_texture_unit);
    // glUniform1i(glGetUniformLocation(program, "emissive_sampler"), gl_emissive_texture_unit); // NOTE: Disabled due to too many FBO attachments used (max 8)
    glUniform1i(glGetUniformLocation(program, "pbr_parameters_sampler"), gl_pbr_parameters_texture_unit);
    glUniform1i(glGetUniformLocation(program, "diffuse_sampler"), gl_diffuse_texture_unit);
    glUniform1i(glGetUniformLocation(program, "geometric_normal_sampler"), gl_geometric_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "tangent_normal_sampler"), gl_tangent_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "tangent_sampler"), gl_tangent_texture_unit);
    glUniform1i(glGetUniformLocation(program, "position_sampler"), gl_position_texture_unit);
    glUniform1i(glGetUniformLocation(program, "shadow_map_sampler"), gl_shadowmapping_texture_unit);
    glUniform1f(glGetUniformLocation(program, "screen_width"), (float) screen.width);
    glUniform1f(glGetUniformLocation(program, "screen_height"), (float) screen.height);
    glUniformMatrix4fv(glGetUniformLocation(program, "light_space_transform"), 1, GL_FALSE, glm::value_ptr(light_space_transform));
    glUniform1i(glGetUniformLocation(program, "shadowmapping"), state.shadowmapping);
    glUniform1i(glGetUniformLocation(program, "normalmapping"), state.normalmapping);

    glUniform3fv(glGetUniformLocation(program, "camera"), 1, &scene->camera->position.x);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    pass_ended();
  }

  /// Copy final pass into default FBO
  {
    pass_started("Final blit pass");

    glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_vct_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    const auto mask = GL_COLOR_BUFFER_BIT;
    const auto filter = GL_NEAREST;
    glBlitFramebuffer(0, 0, screen.width, screen.height, 0, 0, screen.width, screen.height, mask, filter);

    pass_ended();
  }

  log_gl_error();
  state.graphic_batches = graphics_batches.size();

  if (syncs[state.frame % 3]) glDeleteSync(syncs[state.frame % 3]);
  syncs[state.frame % 3] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void Renderer::update_projection_matrix(const float fov, const Resolution& new_screen) {
  // TODO: Adjust all the passes textures sizes & all the global texture buffers
  const float aspect = (float)new_screen.width / (float)new_screen.height;
  this->projection_matrix = glm::perspective(glm::radians(fov), aspect, 0.1f, 3000.0f);
  glViewport(0, 0, new_screen.width, new_screen.height);
  screen = new_screen;
}

void Renderer::link_batch(GraphicsBatch& batch) {
  /// Geometry pass setup
  {
    /// Shaderbindings
    const auto program = batch.depth_shader.gl_program;
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection_matrix));
    
    glUniform1i(glGetUniformLocation(program, "diffuse"), batch.gl_diffuse_texture_unit);
    glUniform1i(glGetUniformLocation(program, "pbr_parameters"), batch.gl_metallic_roughness_texture_unit);
    glUniform1i(glGetUniformLocation(program, "emissive"), batch.gl_emissive_texture_unit);
    glUniform1i(glGetUniformLocation(program, "tangent_normal"), batch.gl_tangent_normal_texture_unit);

    glGenVertexArrays(1, &batch.gl_depth_vao);
    glBindVertexArray(batch.gl_depth_vao);

    glGenBuffers(1, &batch.gl_depth_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_vbo);
    glBufferData(GL_ARRAY_BUFFER, batch.mesh.byte_size_of_vertices(), batch.mesh.vertices.data(), GL_STATIC_DRAW);
		glObjectLabel(GL_BUFFER, batch.gl_depth_vbo, -1, "Batch gl_depth_vbo");

    // Element buffer
    glGenBuffers(1, &batch.gl_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.gl_ebo);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, batch.mesh.byte_size_of_indices(), batch.mesh.indices.data(), 0);
    glObjectLabel(GL_BUFFER, batch.gl_ebo, -1, "Elements SSBO");

    const auto position_attrib = glGetAttribLocation(program, "position");
    glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *) offsetof(Vertex, position));
    glEnableVertexAttribArray(position_attrib);

    const auto normal_attrib = glGetAttribLocation(program, "normal");
    glVertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *) offsetof(Vertex, normal));
    glEnableVertexAttribArray(normal_attrib);

    const auto texcoord_attrib = glGetAttribLocation(program, "texcoord");
    glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *) offsetof(Vertex, tex_coord));
    glEnableVertexAttribArray(texcoord_attrib);

    const auto tangent_attrib = glGetAttribLocation(program, "tangent");
    glVertexAttribPointer(tangent_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(tangent_attrib);

    const auto flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;

    // Bounding volume buffer
    glGenBuffers(1, &batch.gl_bounding_volume_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.gl_bounding_volume_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, GraphicsBatch::INIT_BUFFER_SIZE * sizeof(BoundingVolume), nullptr, flags);
    batch.gl_bounding_volume_buffer_ptr = (uint8_t*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, GraphicsBatch::INIT_BUFFER_SIZE * sizeof(BoundingVolume), flags);
    glObjectLabel(GL_BUFFER, batch.gl_bounding_volume_buffer, -1, "BoundingVolume SSBO");

    // Buffer for all the model matrices
    glGenBuffers(1, &batch.gl_depth_model_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.gl_depth_model_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, GraphicsBatch::INIT_BUFFER_SIZE * sizeof(Mat4f), nullptr, flags);
    batch.gl_depth_model_buffer_ptr = (uint8_t*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, GraphicsBatch::INIT_BUFFER_SIZE * sizeof(Mat4f), flags);
    glObjectLabel(GL_BUFFER, batch.gl_depth_model_buffer, -1, "Model SSBO");

    // Material buffer
    glGenBuffers(1, &batch.gl_material_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.gl_material_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, GraphicsBatch::INIT_BUFFER_SIZE * sizeof(Material), nullptr, flags);
    batch.gl_material_buffer_ptr = (uint8_t*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, GraphicsBatch::INIT_BUFFER_SIZE * sizeof(Material), flags);
    glObjectLabel(GL_BUFFER, batch.gl_material_buffer, -1, "Material SSBO");

    // Setup GL_DRAW_INDIRECT_BUFFER for indirect drawing (basically a command buffer)
    glGenBuffers(1, &batch.gl_ibo);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.gl_ibo);
    glBufferStorage(GL_DRAW_INDIRECT_BUFFER, batch.gl_ibo_count * sizeof(DrawElementsIndirectCommand), nullptr, flags);
    batch.gl_ibo_ptr = (uint8_t*) glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, batch.gl_ibo_count * sizeof(DrawElementsIndirectCommand), flags);
    glObjectLabel(GL_BUFFER, batch.gl_ibo, -1, "Draw Cmd SSBO");

    // Batch instance idx buffer
    glGenBuffers(1, &batch.gl_instance_idx_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.gl_instance_idx_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, GraphicsBatch::INIT_BUFFER_SIZE * sizeof(GLuint), nullptr, 0);
    glObjectLabel(GL_BUFFER, batch.gl_instance_idx_buffer, -1, "Instance idx SSBO");

    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_instance_idx_buffer);
    glVertexAttribIPointer(glGetAttribLocation(program, "instance_idx"), 1, GL_UNSIGNED_INT, sizeof(GLuint), nullptr);
    glEnableVertexAttribArray(glGetAttribLocation(program, "instance_idx"));
    glVertexAttribDivisor(glGetAttribLocation(program, "instance_idx"), 1);
  }
  /// Shadowmap pass setup
  {
    const auto program = shadowmapping_shader->gl_program;
    glUseProgram(program);

    glGenVertexArrays(1, &batch.gl_shadowmapping_vao);
    glBindVertexArray(batch.gl_shadowmapping_vao);

    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_vbo);   // Reuse geometry
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.gl_ebo); // Reuse indices

    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *) offsetof(Vertex, position));
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    
    // Batch instance idx buffer
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_instance_idx_buffer);
    glVertexAttribIPointer(glGetAttribLocation(program, "instance_idx"), 1, GL_UNSIGNED_INT, sizeof(GLuint), nullptr);
    glEnableVertexAttribArray(glGetAttribLocation(program, "instance_idx"));
    glVertexAttribDivisor(glGetAttribLocation(program, "instance_idx"), 1);
  }
  /// Voxelization pass setup
  {
	  const auto program = voxelization_shader->gl_program;
	  glUseProgram(program);

	  glGenVertexArrays(1, &batch.gl_voxelization_vao);
	  glBindVertexArray(batch.gl_voxelization_vao);

	  glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_vbo);   // Reuse geometry
	  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.gl_ebo); // Reuse indices

	  glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));
	  glEnableVertexAttribArray(glGetAttribLocation(program, "position"));

	  glVertexAttribPointer(glGetAttribLocation(program,"normal"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
	  glEnableVertexAttribArray(glGetAttribLocation(program, "normal"));

	  glVertexAttribPointer(glGetAttribLocation(program, "texcoord"), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, tex_coord));
	  glEnableVertexAttribArray(glGetAttribLocation(program, "texcoord"));
    
    // Batch instance idx buffer
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_instance_idx_buffer);
    glVertexAttribIPointer(glGetAttribLocation(program, "instance_idx"), 1, GL_UNSIGNED_INT, sizeof(GLuint), nullptr);
    glEnableVertexAttribArray(glGetAttribLocation(program, "instance_idx"));
    glVertexAttribDivisor(glGetAttribLocation(program, "instance_idx"), 1);
  }
}

void Renderer::add_component(const RenderComponent comp, const ID entity_id) {
  // Handle the config of the Shader from the component
  std::set<Shader::Defines> comp_shader_config;

  if (comp.diffuse_texture.data.pixels) {
    switch (comp.diffuse_texture.gl_texture_target) {
    case GL_TEXTURE_2D_ARRAY:
      comp_shader_config.insert(Shader::Defines::Diffuse2D);
      break;
    case GL_TEXTURE_CUBE_MAP_ARRAY:
      comp_shader_config.insert(Shader::Defines::DiffuseCubemap);
      break;
    default:
      Log::error("Depth shader diffuse texture type not handled.");
    }

    if (comp.diffuse_texture.data.bytes_per_pixel == 3) {
      comp_shader_config.insert(Shader::Defines::DiffuseRGB);
    } else {
      comp_shader_config.insert(Shader::Defines::DiffuseRGBA);
    }
  }

  if (comp.emissive_texture.data.pixels) {
    comp_shader_config.insert(Shader::Defines::Emissive);
  }

  Material material;

  // Shader configuration and mesh id defines the uniqueness of a GBatch
  // FIXME: Does not take every texture into account ...
  for (auto& batch : graphics_batches) {
    if (batch.mesh_id != comp.mesh_id) { continue; }
    if (comp_shader_config != batch.depth_shader.defines) { continue; }
    if (comp.diffuse_texture.data.pixels) {
      const bool batch_contains_texture = batch.layer_idxs.count(comp.diffuse_texture.id) != 0;
      if (batch_contains_texture) {
        material.diffuse_layer_idx = batch.layer_idxs[comp.diffuse_texture.id];
      } else {
        /// Expand texture buffer if needed
        if (batch.diffuse_textures_count + 1 > batch.diffuse_textures_capacity) {
          batch.expand_texture_buffer(comp.diffuse_texture, &batch.gl_diffuse_texture_array, &batch.diffuse_textures_capacity, batch.gl_diffuse_texture_unit);
        }

        /// Update the mapping from texture id to layer idx and increment count
        batch.layer_idxs[comp.diffuse_texture.id] = batch.diffuse_textures_count++;
        material.diffuse_layer_idx = batch.layer_idxs[comp.diffuse_texture.id];

        /// Upload the texture to OpenGL
        batch.upload(comp.diffuse_texture, batch.gl_diffuse_texture_unit, batch.gl_diffuse_texture_array);
      }
    }
    add_graphics_state(batch, comp, material, entity_id);
    return;
  }

  GraphicsBatch batch{comp.mesh_id};

  /// Batch shader prepass (depth pass) shader creation process
  batch.depth_shader = Shader{ Filesystem::base + "shaders/geometry.vert", Filesystem::base + "shaders/geometry.frag" };
  batch.depth_shader.defines = comp_shader_config;

  std::string err_msg;
  bool success;
  std::tie(success, err_msg) = batch.depth_shader.compile();
  if (!success) {
    Log::error("Shader compilation failed; " + err_msg);
    return;
  }

  const uint32_t next_free_texture_unit = Renderer::get_next_free_texture_unit(true);
  if (comp.diffuse_texture.data.pixels) {
    batch.gl_diffuse_texture_unit = next_free_texture_unit;

    batch.init_buffer(comp.diffuse_texture, &batch.gl_diffuse_texture_array, batch.gl_diffuse_texture_unit, &batch.diffuse_textures_capacity);

    /// Update the mapping from texture id to layer idx and increment count
    batch.layer_idxs[comp.diffuse_texture.id] = batch.diffuse_textures_count++;
    material.diffuse_layer_idx = batch.layer_idxs[comp.diffuse_texture.id];

    /// Upload the texture to OpenGL
    batch.upload(comp.diffuse_texture, batch.gl_diffuse_texture_unit, batch.gl_diffuse_texture_array);
  }

  if (comp.metallic_roughness_texture.data.pixels) {
    const Texture& texture = comp.metallic_roughness_texture;
    batch.gl_metallic_roughness_texture_unit = next_free_texture_unit + 1;
    glActiveTexture(GL_TEXTURE0 + batch.gl_metallic_roughness_texture_unit);
    glGenTextures(1, &batch.gl_metallic_roughness_texture);
    glBindTexture(texture.gl_texture_target, batch.gl_metallic_roughness_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(texture.gl_texture_target, 0, GL_RGB, texture.data.width, texture.data.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.data.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  if (comp.normal_texture.data.pixels) {
    const Texture& texture = comp.normal_texture;
    batch.gl_tangent_normal_texture_unit = next_free_texture_unit + 2;
    glActiveTexture(GL_TEXTURE0 + batch.gl_tangent_normal_texture_unit);
    glGenTextures(1, &batch.gl_tangent_normal_texture);
    glBindTexture(texture.gl_texture_target, batch.gl_tangent_normal_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(texture.gl_texture_target, 0, GL_RGB, texture.data.width, texture.data.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.data.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  if (comp.emissive_texture.data.pixels) {
    const Texture& texture = comp.emissive_texture;
    batch.gl_emissive_texture_unit = next_free_texture_unit + 3;
    glActiveTexture(GL_TEXTURE0 + batch.gl_emissive_texture_unit);
    glGenTextures(1, &batch.gl_emissive_texture);
    glBindTexture(texture.gl_texture_target, batch.gl_emissive_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(texture.gl_texture_target, 0, GL_RGB, texture.data.width, texture.data.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.data.pixels);
  }

  link_batch(batch);

  add_graphics_state(batch, comp, material, entity_id);
  graphics_batches.push_back(batch);
}

void Renderer::remove_component(ID entity_id) {
  // TODO: Implement, buffer then components that should be removed
  // components_to_be_removed.push_back(entity_id);
}

void Renderer::add_graphics_state(GraphicsBatch& batch, const RenderComponent& comp, Material material, ID entity_id) {
  if (batch.entity_ids.size() + 1 >= batch.buffer_size) {
    Log::warn("GraphicsBatch MAX_OBJECTS REACHED");
    batch.increase_entity_buffers();
  }

  batch.entity_ids.push_back(entity_id);
  batch.data_idx[entity_id] = batch.entity_ids.size() - 1;

  const TransformComponent transform_comp = TransformSystem::instance().lookup(entity_id);
  const Mat4f transform = compute_transform(transform_comp);
  batch.objects.transforms.push_back(transform);
  uint8_t* dest = batch.gl_depth_model_buffer_ptr + (batch.objects.transforms.size() - 1) * sizeof(Mat4f);
  std::memcpy(dest, &batch.objects.transforms.back(), sizeof(Mat4f));

  // Calculate a bounding volume for the object
  BoundingVolume bounding_volume;
  bounding_volume.radius = batch.bounding_volume.radius * transform_comp.scale;
  bounding_volume.position = Vec3f(transform * Vec4f(batch.bounding_volume.position, 1.0f));
  batch.objects.bounding_volumes.push_back(bounding_volume);
  dest = batch.gl_bounding_volume_buffer_ptr + (batch.objects.bounding_volumes.size() - 1) * sizeof(BoundingVolume);
  std::memcpy(dest, &batch.objects.bounding_volumes.back(), sizeof(BoundingVolume));

  material.pbr_scalar_parameters = Vec2f(comp.pbr_scalar_parameters.y, comp.pbr_scalar_parameters.z);
  material.shading_model = comp.shading_model;

  batch.objects.materials.push_back(material);
  dest = batch.gl_material_buffer_ptr + (batch.objects.materials.size() - 1) * sizeof(Material);
  std::memcpy(dest, &batch.objects.materials.back(), sizeof(Material));
}

void Renderer::update_transforms() {
  const std::vector<ID> t_ids = TransformSystem::instance().get_dirty_transform_ids();
  for (size_t i = 0; i < graphics_batches.size(); i++) {
    auto& batch = graphics_batches[i];
    for (const auto& t_id : t_ids) {
      const auto idx = batch.data_idx.find(t_id);
      if (idx == batch.data_idx.cend()) { continue; }

      // Update the bounding volume for the object
      const TransformComponent transform_comp = TransformSystem::instance().lookup(t_id);
      const Mat4f transform = compute_transform(transform_comp);

      BoundingVolume bounding_volume;
      bounding_volume.radius = batch.bounding_volume.radius * transform_comp.scale;
      bounding_volume.position = Vec3f(transform * Vec4f(batch.bounding_volume.position, 1.0f));
      batch.objects.bounding_volumes[idx->second] = bounding_volume;
      std::memcpy(batch.gl_bounding_volume_buffer_ptr + idx->second * sizeof(BoundingVolume), &batch.objects.bounding_volumes[idx->second], sizeof(BoundingVolume));

      batch.objects.transforms[idx->second] = transform;
      std::memcpy(batch.gl_depth_model_buffer_ptr + idx->second * sizeof(Mat4f), &batch.objects.transforms[idx->second], sizeof(Mat4f));
    }
  }
}
