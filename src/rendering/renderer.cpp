#include "renderer.hpp"
#include "primitives.hpp"

#include <algorithm>
#include <array>
#include <cmath>

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

/// Indicates the start of a Renderpass (must be paried with pass_ended);
inline void Renderer::pass_started(const std::string &name) {
  glBeginQuery(GL_TIME_ELAPSED, gl_query_ids[state.render_passes]);
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
  state.render_passes++;
  if (state.render_passes > MAX_RENDER_PASSES) {
    Log::warn("Maximum render passes");
  }
}

/// Indicates the end of a Renderpass (must be paired with pass_started)
inline void Renderer::pass_ended() const {
  glPopDebugGroup();
  glEndQuery(GL_TIME_ELAPSED);
}

static uint32_t get_next_free_texture_unit(bool peek = false) {
  int32_t max_texture_units = 0;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
  static int32_t next_texture_unit = 0;
  if (peek) {
    return (next_texture_unit + 1) % max_texture_units;
  }
  next_texture_unit++;
  if (next_texture_unit >= max_texture_units) {
    Log::warn("Reached max texture units: " + std::to_string(max_texture_units));
    next_texture_unit = next_texture_unit % max_texture_units;
  }
  return next_texture_unit;
}

static uint32_t get_next_free_image_unit(bool peek = false) {
  int32_t max_image_units = 0;
  glGetIntegerv(GL_MAX_IMAGE_UNITS, &max_image_units);
  static int32_t next_image_unit = -1;
  if (peek) {
    return (next_image_unit + 1) % max_image_units;
  }
  next_image_unit++;
  if (next_image_unit >= max_image_units) {
    Log::warn("Reached max image units: " + std::to_string(max_image_units));
    next_image_unit = next_image_unit % max_image_units;
  }
  return next_image_unit;
}

void Renderer::load_environment_map(const std::array<std::string, 6>& faces) {
  Texture texture;
  const auto resource = TextureResource{{faces[0], faces[1], faces[2], faces[3], faces[4], faces[5]}};
  texture.data = Texture::load_textures(resource);
  if (texture.data.pixels) {
    texture.gl_texture_target = GL_TEXTURE_CUBE_MAP_ARRAY;
    texture.id = resource.to_hash();

    gl_environment_map_texture_unit = get_next_free_texture_unit();
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

// FIXME: Use the same form as above or vice versa
// NOTE: AABB passed is assumed to be the Scene AABB
static glm::mat4 orthographic_projection(const AABB& aabb) {
  const float voxel_grid_dimension = aabb.max_axis() / 2.0f;
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

/// Generates diffuse cones on the hemisphere (Vec3f: cone-direction, float: cone-weight)
/// NOTE: Weights does NOT sum to 2PI, the steradians of a hemisphere, but PI
std::vector<Vec4f> generate_diffuse_cones(const size_t count) {
  assert(count >= 0);

  if (count == 0) {
    return {};
  }

  std::vector<Vec4f> cones(count);

  if (count == 1) {
    cones[0] = Vec4f(0.0f, 1.0f, 0.0f, M_PI);
    return cones;
  }

  const float rad_delta = 2.0f * M_PI / (count - 1.0f);
  const float theta = glm::radians(45.0f);

  // Normal cone
  // NOTE: Weight derived from integration of theta: [0, 2pi], psi: [0, pi/count] of sin(theta) * cos(theta)
  const float w0 = 2.0f * M_PI * (-0.5 * std::cos(M_PI / count) * std::cos(M_PI / count) + 0.5);
  cones[0] = Vec4f(0.0f, 1.0f, 0.0f, w0);

  for (size_t i = 0; i < count - 1; i++) {
    const Vec3f direction = Vec3f(std::cos(theta) * std::sin(i * rad_delta),
                                  std::sin(theta) * std::sin(theta),
                                  std::cos(i * rad_delta)).normalize();
    cones[i + 1] = Vec4f(direction, (M_PI - cones[0].w) / (count - 1.0f));
  }

  float sum = 0.0f;
  for (const auto& cone : cones) {
    sum += cone.w;
    assert(cone.w > 0.0f && "Diffuse cone generated with negative weight.");
  }

  const float tolerance = 0.001f;
  const float diff = std::abs(M_PI - sum);
  assert(diff <= tolerance && "Diffuse cones dont weight up to M_PI");

  return cones;
}

// Center perserved generation of scaled AABBs of the Scene AABB
std::vector<AABB> generate_clipmaps_from_scene_aabb(const AABB& scene,
                                                    const size_t num_clipmaps) {
  assert(num_clipmaps > 1);

  std::vector<AABB> clipmaps(num_clipmaps);
  clipmaps[num_clipmaps - 1] = scene;

  for (size_t i = 0; i < num_clipmaps - 1; i++) {
    const float scaling_factor = 1.0f / std::pow(2.0f, num_clipmaps - i - 2);
    clipmaps[i].scaling_factor = scaling_factor;
    clipmaps[i].max = (scene.max - scene.center()) * scaling_factor + scene.center();
    clipmaps[i].min = (scene.min - scene.center()) * scaling_factor + scene.center();
  }

  return clipmaps;
}

Renderer::~Renderer() = default;

Renderer::Renderer(const Resolution& screen): screen(screen), graphics_batches{} {
  {
    /// Global geometry pass framebuffer
    glGenFramebuffers(1, &gl_depth_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
    glObjectLabel(GL_FRAMEBUFFER, gl_depth_fbo, -1, "GBuffer FBO");

    // Global depth buffer
    gl_depth_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_depth_texture_unit);
    glGenTextures(1, &gl_depth_texture);
    glBindTexture(GL_TEXTURE_2D, gl_depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT); // Default value (intention only to read depth values from texture)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, screen.width, screen.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_depth_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_depth_texture, -1, "GBuffer depth texture");

    // Global normal buffer
    gl_geometric_normal_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_geometric_normal_texture_unit);
    glGenTextures(1, &gl_geometric_normal_texture);
    glBindTexture(GL_TEXTURE_2D, gl_geometric_normal_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_geometric_normal_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_geometric_normal_texture, -1, "GBuffer geometric normal texture");

    // Global position buffer
    gl_position_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_position_texture_unit);
    glGenTextures(1, &gl_position_texture);
    glBindTexture(GL_TEXTURE_2D, gl_position_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gl_position_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_position_texture, -1, "GBuffer position texture");

    // Global diffuse buffer
    gl_diffuse_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_diffuse_texture_unit);
    glGenTextures(1, &gl_diffuse_texture);
    glBindTexture(GL_TEXTURE_2D, gl_diffuse_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gl_diffuse_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_diffuse_texture, -1, "GBuffer diffuse texture");

    // Global PBR parameters buffer
    gl_pbr_parameters_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_pbr_parameters_texture_unit);
    glGenTextures(1, &gl_pbr_parameters_texture);
    glBindTexture(GL_TEXTURE_2D, gl_pbr_parameters_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, gl_pbr_parameters_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_pbr_parameters_texture, -1, "GBuffer PBR parameters texture");

    // Global emissive map
    gl_emissive_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_emissive_texture_unit);
    glGenTextures(1, &gl_emissive_texture);
    glBindTexture(GL_TEXTURE_2D, gl_emissive_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, gl_emissive_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_emissive_texture, -1, "GBuffer emissive texture");

    // Global shading model id
    gl_shading_model_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_shading_model_texture_unit);
    glGenTextures(1, &gl_shading_model_texture);
    glBindTexture(GL_TEXTURE_2D, gl_shading_model_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, screen.width, screen.height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, gl_shading_model_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_shading_model_texture, -1, "GBuffer shading ID texture");

    // Global tangent space normal map
    gl_tangent_normal_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_tangent_normal_texture_unit);
    glGenTextures(1, &gl_tangent_normal_texture);
    glBindTexture(GL_TEXTURE_2D, gl_tangent_normal_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, gl_tangent_normal_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_tangent_normal_texture, -1, "GBuffer tangent normal texture");

    // Global tangent map
    gl_tangent_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_tangent_texture_unit);
    glGenTextures(1, &gl_tangent_texture);
    glBindTexture(GL_TEXTURE_2D, gl_tangent_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT7, gl_tangent_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_tangent_texture, -1, "GBuffer tangent texture");

    uint32_t attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
    glDrawBuffers(std::size(attachments), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("GBuffer framebuffer status not complete.");
    }
  }

  /// Downsample pass for gbuffer
  {
    /// Downsampled global geometry pass framebuffer
    glGenFramebuffers(1, &gbuffer_downsampled.gl_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer_downsampled.gl_fbo);
    glObjectLabel(GL_FRAMEBUFFER, gbuffer_downsampled.gl_fbo, -1, "GBuffer (downsampled) FBO");

    // Global downsampled position buffer
    gbuffer_downsampled.gl_position_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gbuffer_downsampled.gl_position_texture_unit);
    glGenTextures(1, &gbuffer_downsampled.gl_position_texture);
    glBindTexture(GL_TEXTURE_2D, gbuffer_downsampled.gl_position_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer_downsampled.gl_position_texture, 0);
    glObjectLabel(GL_TEXTURE, gbuffer_downsampled.gl_position_texture, -1, "GBuffer (downsampled) position texture");

    // Global downsampled normal buffer
    gbuffer_downsampled.gl_normal_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gbuffer_downsampled.gl_normal_texture_unit);
    glGenTextures(1, &gbuffer_downsampled.gl_normal_texture);
    glBindTexture(GL_TEXTURE_2D, gbuffer_downsampled.gl_normal_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer_downsampled.gl_normal_texture, 0);
    glObjectLabel(GL_TEXTURE, gbuffer_downsampled.gl_normal_texture, -1, "GBuffer (downsampled) normal texture");

    // Global downsampled depth buffer
    gbuffer_downsampled.gl_depth_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gbuffer_downsampled.gl_depth_texture_unit);
    glGenTextures(1, &gbuffer_downsampled.gl_depth_texture);
    glBindTexture(GL_TEXTURE_2D, gbuffer_downsampled.gl_depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, screen.width, screen.height, 0, GL_RED, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gbuffer_downsampled.gl_depth_texture, 0);
    glObjectLabel(GL_TEXTURE, gbuffer_downsampled.gl_depth_texture, -1, "GBuffer (downsampled) depth texture");

    uint32_t attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(std::size(attachments), attachments);

    // NOTE: Reuse shadowmap depth attachment for FBO completeness (disabled anyway)
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_shadowmapping_texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("GBuffer (downsampled) framebuffer status not complete.");
    }

    gbuffer_downsampled.shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                                            Filesystem::base + "shaders/gbuffer-downsampled.frag.glsl");
    const auto [ok, err_msg] = gbuffer_downsampled.shader->compile();
    if (!ok) {
      Log::error("Gbuffer (downsampled) shader error: " + err_msg); exit(1);
    }

    const auto program = gbuffer_downsampled.shader->gl_program;
    glUseProgram(program);

    glGenVertexArrays(1, &gbuffer_downsampled.gl_vao);
    glBindVertexArray(gbuffer_downsampled.gl_vao);

    GLuint gl_vbo = 0;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  }

  // TODO: Rework pointlights
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
  cull_shader = new ComputeShader(Filesystem::base + "shaders/culling.comp.glsl");

  /// Directional shadow mapping setup
  {
    glGenFramebuffers(1, &gl_shadowmapping_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_shadowmapping_fbo);
		glObjectLabel(GL_FRAMEBUFFER, gl_shadowmapping_fbo, -1, "Shadowmap FBO");

    gl_shadowmapping_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_shadowmapping_texture_unit); // FIXME: These are not neccessary when creating the texture only when they are used
    glGenTextures(1, &gl_shadowmapping_texture);
    glBindTexture(GL_TEXTURE_2D, gl_shadowmapping_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, state.shadow.SHADOWMAP_W, state.shadow.SHADOWMAP_H);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_shadowmapping_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_shadowmapping_texture, -1, "Shadowmap texture");
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Directional shadow mapping FBO not complete"); exit(1);
    }

    shadowmapping_shader = new Shader(Filesystem::base + "shaders/shadowmapping.vert",
                                      Filesystem::base + "shaders/shadowmapping.frag");
    const auto [ok, err_msg] = shadowmapping_shader->compile();
    if (!ok) {
      Log::error("Shadowmapping shader error: " + err_msg); exit(1);
    }
  }

  /// Voxelization pass
  {
    voxelization_shader = new Shader(Filesystem::base + "shaders/voxelization.vert",
                                     Filesystem::base + "shaders/voxelization.geom",
                                     Filesystem::base + "shaders/voxelization.frag");

    const std::string includes = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
    voxelization_shader->add(includes);

    const auto [ok, err_msg] = voxelization_shader->compile();
    if (!ok) {
      Log::error("Voxelization shader error: " + err_msg); exit(-1);
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
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

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
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

      const std::string opacity_object_label = "Clipmap #" + std::to_string(i) + " opacity texture";
      glObjectLabel(GL_TEXTURE, gl_voxel_opacity_textures[i], -1, opacity_object_label.c_str());
    }

    // Reuse shadowmap depth attachment for FBO completeness (disabled anyway)
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_shadowmapping_texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Voxelization FBO not complete"); exit(1);
    }

    /// Opacity normalization compute shader subpass
    const std::string include = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
    voxelization_opacity_norm_shader = new ComputeShader(Filesystem::base + "shaders/voxelization-opacity-normalization.comp",
                                                         std::vector{include});
  }

  /// Voxel visualization pass
  if (state.voxel_visualization.enabled) {
    voxel_visualization_shader = new Shader(Filesystem::base + "shaders/voxel-visualization.vert",
                                            Filesystem::base + "shaders/voxel-visualization.geom",
                                            Filesystem::base + "shaders/voxel-visualization.frag");

    const std::string includes = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
    voxel_visualization_shader->add(includes);

    const auto [ok, err_msg] = voxel_visualization_shader->compile();

    if (!ok) {
      Log::error("Voxel visualization shader error: " + err_msg); exit(-1);
    } 

    const auto program = voxel_visualization_shader->gl_program;
    glUseProgram(program);

    glGenFramebuffers(1, &gl_voxel_visualization_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_voxel_visualization_fbo);
    glObjectLabel(GL_FRAMEBUFFER, gl_voxel_visualization_fbo, -1, "Voxel visualization FBO");

    gl_voxel_visualization_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_voxel_visualization_texture_unit);
    glGenTextures(1, &gl_voxel_visualization_texture);
    glBindTexture(GL_TEXTURE_2D, gl_voxel_visualization_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_voxel_visualization_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_voxel_visualization_texture, -1, "Voxel visualization texture");

    const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(std::size(attachments), attachments);

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
    const size_t MAX_CONES = state.vct.MAX_DIFFUSE_CONES;
    const std::vector<Vec4f> cones = generate_diffuse_cones(state.vct.num_diffuse_cones);

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

    const std::string includes = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
    vct_shader->add(includes);

    const auto [ok, err_msg] = vct_shader->compile();
    if (!ok) {
      Log::error("Voxel cone tracing shader error: " + err_msg); exit(-1);
    }

    const auto program = vct_shader->gl_program;
    glUseProgram(program);

    glGenFramebuffers(1, &gl_vct_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_vct_fbo);

    // Global indirect radiance map
    gl_indirect_radiance_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_indirect_radiance_texture_unit);
    glGenTextures(1, &gl_indirect_radiance_texture);
    glBindTexture(GL_TEXTURE_2D, gl_indirect_radiance_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_indirect_radiance_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_indirect_radiance_texture, -1, "GBuffer indirect radiance texture");

    // Global ambient radiance map
    gl_ambient_radiance_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_ambient_radiance_texture_unit);
    glGenTextures(1, &gl_ambient_radiance_texture);
    glBindTexture(GL_TEXTURE_2D, gl_ambient_radiance_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, screen.width, screen.height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gl_ambient_radiance_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_ambient_radiance_texture, -1, "GBuffer ambient radiance texture");

    // Global specular radiance map
    gl_specular_radiance_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_specular_radiance_texture_unit);
    glGenTextures(1, &gl_specular_radiance_texture);
    glBindTexture(GL_TEXTURE_2D, gl_specular_radiance_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gl_specular_radiance_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_specular_radiance_texture, -1, "GBuffer specular radiance texture");

    // Global direct radiance map
    gl_direct_radiance_texture_unit = get_next_free_texture_unit();
    glActiveTexture(GL_TEXTURE0 + gl_direct_radiance_texture_unit);
    glGenTextures(1, &gl_direct_radiance_texture);
    glBindTexture(GL_TEXTURE_2D, gl_direct_radiance_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, gl_direct_radiance_texture, 0);
    glObjectLabel(GL_TEXTURE, gl_direct_radiance_texture, -1, "GBuffer direct radiance texture");

    const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(std::size(attachments), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("VCT fbo not complete"); log_gl_error(); exit(-1);
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

  /// Bilateral filtering pass
  {
    bf_ping_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                                Filesystem::base + "shaders/bilateral-filtering.frag.glsl");

    // NOTE: Include order matters
    const std::string include0 = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
    const std::string include1 = Filesystem::read_file(Filesystem::base + "shaders/bilateral-filtering-utils.glsl");
    bf_ping_shader->add(include1);
    bf_ping_shader->add(include0);
    bf_ping_shader->add("#define VERTICAL_STEP_DIR \n");

    const auto [ok, msg] = bf_ping_shader->compile();
    if (!ok) {
      Log::error(msg); exit(-1);
    }

    bf_pong_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                                Filesystem::base + "shaders/bilateral-filtering.frag.glsl");

    bf_pong_shader->add(include1);
    bf_pong_shader->add(include0);
    bf_pong_shader->add("#define HORIZONTAL_STEP_DIR \n");

    const auto [ok_pong, msg_pong] = bf_pong_shader->compile();
    if (!ok_pong) {
      Log::error(msg_pong); exit(-1);
    }

    // Ping buffer
    {
      const uint32_t program = bf_ping_shader->gl_program;
      glUseProgram(program);
      glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
      glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

      GLuint gl_vbo = 0;
      glGenBuffers(1, &gl_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

      glGenFramebuffers(1, &gl_bf_ping_fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_bf_ping_fbo);

      gl_bf_ping_out_texture_unit = get_next_free_texture_unit();
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
      const uint32_t program = bf_pong_shader->gl_program;
      glUseProgram(program);
      glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
      glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

      GLuint gl_vbo = 0;
      glGenBuffers(1, &gl_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

      glGenFramebuffers(1, &gl_bf_pong_fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_bf_pong_fbo);

      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_ambient_radiance_texture, 0); // NOTE: Use w/e temp. texture for FBO completeness

      const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(std::size(attachments), attachments);

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Log::error("Bilateral filtering pong FBO not complete"); exit(-1);
      }
    }
  }

  /// Lighting application pass
  {
    lighting_application_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                                             Filesystem::base + "shaders/lighting-application.frag.glsl");

    const std::string includes = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
    lighting_application_shader->add(includes);

    const auto [ok, msg] = lighting_application_shader->compile();
    if (!ok) {
      Log::error(msg); exit(-1);
    }

    const uint32_t program = lighting_application_shader->gl_program;
    glUseProgram(program);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    GLuint gl_vbo = 0;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

    glGenFramebuffers(1, &gl_lighting_application_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_lighting_application_fbo);

    gl_lighting_application_texture_unit = get_next_free_texture_unit();
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
      Log::error("Lighting application FBO not complete"); exit(-1);
    }
  }

  /// Bilateral upsampling
  {
    bs_ping_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                                Filesystem::base + "shaders/bilateral-upsampling.frag.glsl");

    // NOTE: Include order matters
    const std::string include0 = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
    const std::string include1 = Filesystem::read_file(Filesystem::base + "shaders/bilateral-filtering-utils.glsl");
    bs_ping_shader->add(include1);
    bs_ping_shader->add(include0);

    const auto [ok, msg] = bs_ping_shader->compile();
    if (!ok) {
      Log::error(msg); exit(-1);
    }

    // Ping buffer
    {
      const uint32_t program = bs_ping_shader->gl_program;
      glUseProgram(program);
      glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
      glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

      GLuint gl_vbo = 0;
      glGenBuffers(1, &gl_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

      glGenFramebuffers(1, &gl_bs_ping_fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_bs_ping_fbo);

      gl_bs_ping_out_texture_unit = get_next_free_texture_unit();
      glActiveTexture(GL_TEXTURE0 + gl_bs_ping_out_texture_unit);

      glGenTextures(1, &gl_bs_ping_out_texture);
      glBindTexture(GL_TEXTURE_2D, gl_bs_ping_out_texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen.width, screen.height, 0, GL_RGBA, GL_FLOAT, nullptr);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_bs_ping_out_texture, 0);
      glObjectLabel(GL_TEXTURE, gl_bs_ping_out_texture, -1, "Bilateral upsampling ping output texture");

      const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(std::size(attachments), attachments);

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Log::error("Bilateral upsampling ping FBO not complete"); exit(-1);
      }
    }
  }

  /// Bilinear upsampling pass
  {
    bilinear_upsampling_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                                            Filesystem::base + "shaders/bilinear-upsampling.frag.glsl");

    const auto [ok, msg] = bilinear_upsampling_shader->compile();
    if (!ok) {
      Log::error(msg); exit(-1);
    }

    const uint32_t program = bilinear_upsampling_shader->gl_program;
    glUseProgram(program);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    GLuint gl_vbo = 0;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

    glGenFramebuffers(1, &gl_bilinear_upsampling_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_bilinear_upsampling_fbo);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_bf_ping_out_texture, 0);

    const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(std::size(attachments), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Bilinear upsampling FBO not complete"); exit(-1);
    }
  }

  /// Direct lighting pass
  {
    direct_lighting_shader = new Shader(Filesystem::base + "shaders/generic-passthrough.vert.glsl",
                                        Filesystem::base + "shaders/direct-lighting.frag.glsl");

    const auto [ok, msg] = direct_lighting_shader->compile();
    if (!ok) {
      Log::error(msg); exit(-1);
    }

    const uint32_t program = direct_lighting_shader->gl_program;
    glUseProgram(program);
    glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
    glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    GLuint gl_vbo = 0;
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);

    glGenFramebuffers(1, &gl_direct_lighting_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_direct_lighting_fbo);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_direct_radiance_texture, 0);

    const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(std::size(attachments), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      Log::error("Direct lighting FBO not complete"); exit(-1);
    }
  }

  /// Rendering pass execution time query buffer
  {
    glGenQueries(MAX_RENDER_PASSES, &gl_query_ids[0]);

    glGenBuffers(1, &gl_execution_time_query_buffer);
    glBindBuffer(GL_QUERY_BUFFER, gl_execution_time_query_buffer);
    const auto flags = GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT;
    glBufferStorage(GL_QUERY_BUFFER, MAX_RENDER_PASSES * sizeof(GLuint64), nullptr, flags);
    gl_query_time_buffer_ptr = (uint64_t*) glMapBufferRange(GL_QUERY_BUFFER, 0, MAX_RENDER_PASSES * sizeof(GLuint64), flags);
    glObjectLabel(GL_BUFFER, gl_execution_time_query_buffer, -1, "Renderpass execution time buffer");
  }

  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
}

// Src: https://github.com/g-truc/glm/blob/0.9.5/glm/gtc/matrix_transform.inl
static Mat4f perspective(const float fovy, const Resolution& screen, const float z_near, const float z_far) {
  assert(fovy != 0.0f);
  assert(z_near != z_far);

  const float aspect = (float)screen.width / (float)screen.height;
  const float tan_half_fov = std::tan(fovy / 2.0f);

  Mat4f m(0.0f);
  m[0][0] = 1.0f / (aspect * tan_half_fov);
  m[1][1] = 1.0f / tan_half_fov;
  m[2][2] = - (z_far + z_near) / (z_far - z_near);
  m[2][3] = 1.0f;
  m[3][2] = (2.0f * z_far * z_near) / (z_far + z_near);
  return m;
}

bool Renderer::init() {
  const std::vector<AABB> aabbs = generate_clipmaps_from_scene_aabb(scene->aabb, NUM_CLIPMAPS);
  for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
    clipmaps.aabb[i] = aabbs[i];
    Log::info("--------------------");
    Log::info(aabbs[i]);
    Log::info("AABB center: "   + aabbs[i].center().to_string());
    Log::info("AABB max axis: " + std::to_string(aabbs[i].max_axis()));
    Log::info("AABB scaling_factor: " + std::to_string(aabbs[i].scaling_factor));
    Log::info("Voxel size: "    + std::to_string(aabbs[i].max_axis() / clipmaps.size[i]));
    Log::info("Voxel d^3: "     + std::to_string(clipmaps.size[i]));
  }
  return true;
}

void Renderer::render(const uint32_t delta) {
  state.frame++;
  state.render_passes = 0;
  state.total_execution_time = 0;

  // Asserts downsampling factor
  if (state.lighting.downsample_modifier % 2 != 0 || state.lighting.downsample_modifier <= 0) {
    state.lighting.downsample_modifier = 1;
  }

  // Help variable for the indexing of executing timings of the render passes
  const uint8_t TIME_IDX = state.frame % RenderState::execution_time_buffer_size;

  GLuint last_executed_fbo = 0; // NOTE: This FBO's default buffer is blitted to the screen

  const float aspect_ratio = static_cast<float>(screen.width) / static_cast<float>(screen.height);
  const auto projection_matrix = scene->camera.projection(aspect_ratio);

  const glm::mat4 camera_transform = projection_matrix * scene->camera.transform(); // TODO: Camera handling needs to be reworked

  /// Renderer caches the transforms of components thus we need to fetch the ones who changed during the last frame
  // FIXME: This is a bad design?
  update_transforms();
  TransformSystem::instance().reset_dirty();

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

  if (state.culling.enabled) {
    state.culling.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
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

      glUniform1ui(glGetUniformLocation(cull_shader->gl_program, "NUM_INDICES"), batch.mesh->indices.size());
      glUniform1ui(glGetUniformLocation(cull_shader->gl_program, "DRAW_CMD_IDX"), batch.gl_curr_ibo_idx);

      glDispatchCompute(batch.objects.transforms.size(), 1, 1);
    }
    // TODO: Is this barrier required?
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT); // Buffer objects affected by this bit are derived from the GL_DRAW_INDIRECT_BUFFER binding.

    pass_ended();
  }

  state.shadow.execution_time_shadowmapping[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
  pass_started("Directional shadow mapping pass");
  const glm::mat4 light_space_transform = shadowmap_transform(scene->aabb, scene->directional_light);
  {
    glCullFace(GL_FRONT);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_shadowmapping_fbo);
    glViewport(0, 0, state.shadow.SHADOWMAP_W, state.shadow.SHADOWMAP_H);
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
    state.gbuffer.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Geometry pass");

    glBindFramebuffer(GL_FRAMEBUFFER, gl_depth_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (size_t i = 0; i < graphics_batches.size(); i++) {
      const auto& batch = graphics_batches[i];
      const auto program = batch.depth_shader.gl_program;
      glUseProgram(program);
      glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection_matrix));
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
        glActiveTexture(GL_TEXTURE0 + batch.gl_metallic_roughness_texture_unit); // TODO: Replace with DSA
        glBindTexture(GL_TEXTURE_2D, batch.gl_metallic_roughness_texture);
      }

      if (batch.gl_tangent_normal_texture != 0) {
        glActiveTexture(GL_TEXTURE0 + batch.gl_tangent_normal_texture_unit); // TODO: Replace with DSA
        glBindTexture(GL_TEXTURE_2D, batch.gl_tangent_normal_texture);
      }

      glActiveTexture(GL_TEXTURE0 + batch.gl_emissive_texture_unit); // TODO: Replace with DSA
      glBindTexture(GL_TEXTURE_2D, batch.gl_emissive_texture);

      const uint64_t draw_cmd_offset = batch.gl_curr_ibo_idx * sizeof(DrawElementsIndirectCommand);
      glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*) draw_cmd_offset, 1, sizeof(DrawElementsIndirectCommand));
    }

    pass_ended();
  }

  /// Downsample pass
  {
    state.gbuffer_downsample.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Downsample pass");

    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer_downsampled.gl_fbo);
    glBindVertexArray(gbuffer_downsampled.gl_vao);

    const auto program = gbuffer_downsampled.shader->gl_program;
    glUseProgram(program);

    const auto div = state.lighting.downsample_modifier;
    const Vec2f input_pixel_size = Vec2(float(div) / (screen.width), float(div) / (screen.height));
    glUniform2fv(glGetUniformLocation(program, "uInput_pixel_size"), 1, &input_pixel_size.x);

    glUniform1i(glGetUniformLocation(program, "uPosition"), gl_position_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uNormal"), gl_geometric_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uDepth"), gl_depth_texture_unit);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, screen.width / div, screen.height / div);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glViewport(0, 0, screen.width, screen.height);

    pass_ended();
  }

  if (state.voxelization.voxelize) {
    if (!state.voxelization.always_voxelize) {
      state.voxelization.voxelize = false;
    }

    state.voxelization.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Voxelization pass");

    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      glClearTexImage(gl_voxel_radiance_textures[i], 0, GL_RGBA, GL_FLOAT, nullptr); 
      glClearTexImage(gl_voxel_opacity_textures[i], 0, GL_RGBA, GL_FLOAT, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, gl_voxelization_fbo);

    const auto program = voxelization_shader->gl_program;
    glUseProgram(program);

    // Orthogonal projection along +z-axis
    // TODO: Precompute these - add notification when changed and precompute the new ones
    glm::mat4 orthos[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      orthos[i] = orthographic_projection(clipmaps.aabb[i]);
    }
    glUniformMatrix4fv(glGetUniformLocation(program, "uOrthos"), NUM_CLIPMAPS, GL_FALSE, glm::value_ptr(orthos[0]));

    // TODO: Precompute these - add notification when changed and precompute the new ones
    float scaling_factors[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      scaling_factors[i] = 1.0f / clipmaps.aabb[i].max_axis();
    }
    glUniform1fv(glGetUniformLocation(program, "uScaling_factors"), NUM_CLIPMAPS, scaling_factors);

    // TODO: Precompute these - add notification when changed and precompute the new ones
    Vec3f aabb_centers[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      aabb_centers[i] = clipmaps.aabb[i].center();
    }
    glUniform3fv(glGetUniformLocation(program, "uAABB_centers"), NUM_CLIPMAPS, &aabb_centers[0].x);
    glUniform3fv(glGetUniformLocation(program, "uCamera_position"), 1, &scene->camera.position.x);

    // Shadowmapping
    glUniform1f(glGetUniformLocation(program, "uShadow_bias"), state.shadow.bias);
    glUniform3fv(glGetUniformLocation(program, "uDirectional_light_direction"), 1, &scene->directional_light.direction.x);
    glUniform3fv(glGetUniformLocation(program, "uDirectional_light_intensity"), 1, &scene->directional_light.intensity.x);
    glUniformMatrix4fv(glGetUniformLocation(program, "uLight_space_transform"), 1, GL_FALSE, glm::value_ptr(light_space_transform));
    glUniform1i(glGetUniformLocation(program, "uShadowmap"), gl_shadowmapping_texture_unit);
    glUniform1iv(glGetUniformLocation(program, "uClipmap_sizes"), NUM_CLIPMAPS, clipmaps.size);
    glUniform1i(glGetUniformLocation(program, "uConservative_rasterization_enabled"), state.voxelization.conservative_rasterization);

    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      glBindImageTexture(gl_voxel_radiance_image_units[i], gl_voxel_radiance_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
      glBindImageTexture(gl_voxel_opacity_image_units[i], gl_voxel_opacity_textures[i], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
    }
    glUniform1iv(glGetUniformLocation(program, "uVoxel_radiance"), NUM_CLIPMAPS, gl_voxel_radiance_image_units);
    glUniform1iv(glGetUniformLocation(program, "uVoxel_opacity"), NUM_CLIPMAPS, gl_voxel_opacity_image_units);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    // TODO: Precompute these - add notification when changed and precompute the new ones
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

      const uint32_t gl_material_binding_point = 3; // Defaults to 3 in geometry.frag shader
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_material_binding_point, batch.gl_material_buffer);

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
      state.voxelization.execution_time[TIME_IDX] += gl_query_time_buffer_ptr[state.render_passes];
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

  // FIXME: Does not work
  if (state.voxel_visualization.enabled) {
    state.voxel_visualization.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Voxel visualization pass");

    const auto program = voxel_visualization_shader->gl_program;
    glUseProgram(program);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_voxel_visualization_fbo);
    glBindVertexArray(gl_voxel_visualization_vao);

    glActiveTexture(GL_TEXTURE0 + gl_voxel_visualization_texture_unit);
    glBindTexture(GL_TEXTURE_2D, gl_voxel_visualization_texture);

    glUniform1iv(glGetUniformLocation(program, "uClipmap_sizes"), NUM_CLIPMAPS, clipmaps.size);
    glUniformMatrix4fv(glGetUniformLocation(program, "uCamera_view"), 1, GL_FALSE, glm::value_ptr(camera_transform));

    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      glBindImageTexture(gl_voxel_radiance_image_units[i], gl_voxel_radiance_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
      glBindImageTexture(gl_voxel_opacity_image_units[i], gl_voxel_opacity_textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    }
    glUniform1iv(glGetUniformLocation(program, "uVoxel_opacity"), NUM_CLIPMAPS, gl_voxel_opacity_image_units);
    glUniform1iv(glGetUniformLocation(program, "uVoxel_radiance"), NUM_CLIPMAPS, gl_voxel_radiance_image_units);

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

  /// Voxel cone tracing pass
  {
    state.vct.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Voxel cone tracing pass");
    const auto program = vct_shader->gl_program;

    glUseProgram(program);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_vct_fbo);
    glBindVertexArray(gl_vct_vao);

    // User customizable
    glUniform1i(glGetUniformLocation(program, "uIndirect"), state.lighting.indirect);
    glUniform1i(glGetUniformLocation(program, "uEmissive"), state.lighting.emissive);
    glUniform1i(glGetUniformLocation(program, "uSpecular"), state.lighting.specular);
    glUniform1i(glGetUniformLocation(program, "uAmbient"), state.lighting.ambient);
    glUniform1i(glGetUniformLocation(program, "uDirect"), state.lighting.direct && state.shadow.algorithm == ShadowAlgorithm::VCT);
    const float roughness_aperature = glm::radians(state.vct.roughness_aperature);
    glUniform1f(glGetUniformLocation(program, "uRoughness_aperature"), roughness_aperature);
    const float metallic_aperature = glm::radians(state.vct.metallic_aperature / 2.0f);
    glUniform1f(glGetUniformLocation(program, "uMetallic_aperature"), metallic_aperature);
    glUniform1f(glGetUniformLocation(program, "uAmbient_decay"), state.vct.ambient_decay);
    glUniform1f(glGetUniformLocation(program, "uSpecular_cone_trace_distance"), state.vct.specular_cone_trace_distance);

    glUniform3fv(glGetUniformLocation(program, "uCamera_position"), 1, &scene->camera.position.x);

    glUniform1ui(glGetUniformLocation(program, "uNum_diffuse_cones"), state.vct.num_diffuse_cones);
    // TODO: Precompute these - add notification when changed and precompute the new ones
    const std::vector<Vec4f> cones = generate_diffuse_cones(state.vct.num_diffuse_cones);
    std::memcpy(gl_vct_diffuse_cones_ssbo_ptr, cones.data(), cones.size() * sizeof(Vec4f));

    glUniform1i(glGetUniformLocation(program, "uNormalmapping"), state.lighting.normalmapping);

    // TODO: Precompute these - add notification when changed and precompute the new ones
    float scaling_factors[NUM_CLIPMAPS];
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      scaling_factors[i] = 1.0f / clipmaps.aabb[i].max_axis();
    }
    glUniform1fv(glGetUniformLocation(program, "uScaling_factors"), NUM_CLIPMAPS, scaling_factors);

    const float voxel_size_LOD0 = clipmaps.aabb[0].max_axis() / float(clipmaps.size[0]);
    glUniform1f(glGetUniformLocation(program, "uVoxel_size_LOD0"), voxel_size_LOD0);

    // TODO: Precompute these - add notification when changed and precompute the new ones
    Vec3f clipmap_aabb_centers[NUM_CLIPMAPS];
    Vec3f clipmap_aabb_mins[NUM_CLIPMAPS];
    Vec3f clipmap_aabb_maxs[NUM_CLIPMAPS];

    // TODO: Precompute these - add notification when changed and precompute the new ones
    for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
      clipmap_aabb_centers[i] = clipmaps.aabb[i].center();
      clipmap_aabb_mins[i] = clipmaps.aabb[i].min;
      clipmap_aabb_maxs[i] = clipmaps.aabb[i].max;
    }

    glUniform3fv(glGetUniformLocation(program, "uAABB_centers"), NUM_CLIPMAPS, &clipmap_aabb_centers[0].x);
    glUniform3fv(glGetUniformLocation(program, "uAABB_mins"), NUM_CLIPMAPS, &clipmap_aabb_mins[0].x);
    glUniform3fv(glGetUniformLocation(program, "uAABB_maxs"), NUM_CLIPMAPS, &clipmap_aabb_maxs[0].x);
		glUniform1iv(glGetUniformLocation(program, "uVoxel_radiance"), NUM_CLIPMAPS, gl_voxel_radiance_texture_units);
    glUniform1iv(glGetUniformLocation(program, "uVoxel_opacity"), NUM_CLIPMAPS, gl_voxel_opacity_texture_units);
    glUniform1ui(glGetUniformLocation(program, "uScreen_width"), screen.width / state.lighting.downsample_modifier);
    glUniform1ui(glGetUniformLocation(program, "uScreen_height"), screen.height / state.lighting.downsample_modifier);
    glUniform1i(glGetUniformLocation(program, "uPosition"), gl_position_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uNormal"), gl_geometric_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uPBR_parameters"), gl_pbr_parameters_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gl_tangent_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uTangent"), gl_tangent_texture_unit);

    glViewport(0, 0, screen.width / state.lighting.downsample_modifier, screen.height / state.lighting.downsample_modifier);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glViewport(0, 0, screen.width, screen.height);

    last_executed_fbo = gl_vct_fbo;

    pass_ended();
  }

  /// Direct lighting (non-VCT) pass
  if (state.lighting.direct && state.shadow.algorithm != ShadowAlgorithm::VCT) {
    state.shadow.execution_time_shadow[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Direct lighting pass");

    const auto program = direct_lighting_shader->gl_program;
    glUseProgram(program);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_direct_lighting_fbo);

    glUniform1ui(glGetUniformLocation(program, "uPCF_samples"), state.shadow.pcf_samples);
    glUniform1f(glGetUniformLocation(program, "uVCT_shadow_cone_aperature"), state.shadow.vct_cone_aperature);
    glUniform1ui(glGetUniformLocation(program, "uShadow_algorithm"), static_cast<uint32_t>(state.shadow.algorithm));
    glUniform1f(glGetUniformLocation(program, "uShadow_bias"), state.shadow.bias);
    glUniform3fv(glGetUniformLocation(program, "uDirectional_light_intensity"), 1, &scene->directional_light.intensity.x);
    glUniform3fv(glGetUniformLocation(program, "uDirectional_light_direction"), 1, &scene->directional_light.direction.x);
    glUniformMatrix4fv(glGetUniformLocation(program, "uLight_space_transform"), 1, GL_FALSE, glm::value_ptr(light_space_transform));
    glUniform1i(glGetUniformLocation(program, "uShadowmap"), gl_shadowmapping_texture_unit);
    glUniform1ui(glGetUniformLocation(program, "uShadowmap_width"), state.shadow.SHADOWMAP_W);
    glUniform1ui(glGetUniformLocation(program, "uShadowmap_height"), state.shadow.SHADOWMAP_H);

    glUniform1ui(glGetUniformLocation(program, "uScreen_width"), screen.width);
    glUniform1ui(glGetUniformLocation(program, "uScreen_height"), screen.height);
    glUniform1i(glGetUniformLocation(program, "uPosition"), gl_position_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uNormal"), gl_geometric_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gl_tangent_normal_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uTangent"), gl_tangent_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uNormalmapping"), state.lighting.normalmapping);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    last_executed_fbo = gl_direct_lighting_fbo;

    pass_ended();
  }

  state.bilateral_filtering.kernel = gaussian_1d_kernel(state.bilateral_filtering.spatial_kernel_sigma, state.bilateral_filtering.spatial_kernel_radius);
  if (state.bilateral_filtering.enabled && state.lighting.downsample_modifier > 1) {
    state.bilateral_filtering.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Bilateral filtering pass");

    // NOTE: Joint bilateral filtering of a noisy signal with one or more 'robust' signals
    const auto bilateral_filtering_pass = [&](const uint32_t in_texture, const uint32_t in_texture_unit, const float div = 1.0f) {
      // TODO: Reduce to one single shader, reuse that one twice instead
      // Ping
      {
        const auto program = bf_ping_shader->gl_program;
        glUseProgram(program);
        glBindFramebuffer(GL_FRAMEBUFFER, gl_bf_ping_fbo);

        glTextureParameteri(in_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(in_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTextureParameteri(gl_geometric_normal_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(gl_geometric_normal_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTextureParameteri(gl_position_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(gl_position_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTextureParameteri(gl_depth_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(gl_depth_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTextureParameteri(gl_bf_ping_out_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(gl_bf_ping_out_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glUniform1i(glGetUniformLocation(program, "uPosition_weight"), state.bilateral_filtering.position_weight);
        glUniform1i(glGetUniformLocation(program, "uPosition"), gl_position_texture_unit);
        glUniform1f(glGetUniformLocation(program, "uPosition_sigma"), state.bilateral_filtering.position_sigma);
        glUniform1i(glGetUniformLocation(program, "uNormal_weight"), state.bilateral_filtering.normal_weight);
        glUniform1i(glGetUniformLocation(program, "uNormal"), gl_geometric_normal_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uTangent"), gl_tangent_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gl_tangent_normal_texture_unit);
        glUniform1f(glGetUniformLocation(program, "uNormal_sigma"), state.bilateral_filtering.normal_sigma);
        glUniform1i(glGetUniformLocation(program, "uDepth_weight"), state.bilateral_filtering.depth_weight);
        glUniform1i(glGetUniformLocation(program, "uDepth"), gl_depth_texture_unit);
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
        glUniform1i(glGetUniformLocation(program, "uPosition"), gl_position_texture_unit);
        glUniform1f(glGetUniformLocation(program, "uPosition_sigma"), state.bilateral_filtering.position_sigma);
        glUniform1i(glGetUniformLocation(program, "uNormal_weight"), state.bilateral_filtering.normal_weight);
        glUniform1i(glGetUniformLocation(program, "uNormal"), gl_geometric_normal_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gl_tangent_normal_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uTangent"), gl_tangent_texture_unit);
        glUniform1f(glGetUniformLocation(program, "uNormal_sigma"), state.bilateral_filtering.normal_sigma);
        glUniform1i(glGetUniformLocation(program, "uDepth_weight"), state.bilateral_filtering.depth_weight);
        glUniform1i(glGetUniformLocation(program, "uDepth"), gl_depth_texture_unit);
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

    // Pre-filtering screenshot
    const auto get_texture = [&](const uint32_t gl_texture) -> Vec3f* {
      const size_t buf_size = sizeof(Vec3f) * screen.width * screen.height;
      Vec3f *pixels = (Vec3f*) calloc(buf_size, 1);
      glGetTextureImage(gl_texture, 0, GL_RGB, GL_FLOAT, buf_size, (void*) pixels);
      return pixels;
    };

    Vec3f* ambient_radiance_pixels = nullptr;
    Vec3f* indirect_radiance_pixels = nullptr;
    Vec3f* specular_radiance_pixels = nullptr;
    if (state.bilateral_filtering.pixel_diff) {
      if (state.bilateral_filtering.ambient)  { ambient_radiance_pixels = get_texture(gl_ambient_radiance_texture); };
      if (state.bilateral_filtering.indirect) { indirect_radiance_pixels = get_texture(gl_indirect_radiance_texture); };
      if (state.bilateral_filtering.specular) { specular_radiance_pixels = get_texture(gl_specular_radiance_texture); };
    }

    // Declare input & output texture
    const float div = state.lighting.downsample_modifier;
    if (state.bilateral_filtering.ambient)  { bilateral_filtering_pass(gl_ambient_radiance_texture,  gl_ambient_radiance_texture_unit, div);  }
    if (state.bilateral_filtering.indirect) { bilateral_filtering_pass(gl_indirect_radiance_texture, gl_indirect_radiance_texture_unit, div); }
    if (state.bilateral_filtering.specular) { bilateral_filtering_pass(gl_specular_radiance_texture, gl_specular_radiance_texture_unit, div); }
    if (state.bilateral_filtering.direct)   { bilateral_filtering_pass(gl_direct_radiance_texture,   gl_direct_radiance_texture_unit);   }

    const auto save_pixel_diff = [&](const std::string& filename, const Vec3f* pre, const Vec3f* post, const TextureFormat fmt = TextureFormat::RGB32F) {
      const std::string pre_fp  = Filesystem::save_image_as_ppm(Filesystem::tmp + "pre-"  + filename, pre, screen.width, screen.height, div, fmt);
      const std::string post_fp = Filesystem::save_image_as_ppm(Filesystem::tmp + "post-" + filename, post, screen.width, screen.height, div, fmt);
      const std::string out_fp = Filesystem::tmp + "diff-" + filename + ".png";
      // FIXME: Compare using grayscale not binary white-black?
      const std::string cmd = "compare -highlight-color white " + pre_fp + " " + post_fp + " " + out_fp;
      std::system(cmd.c_str());
      Log::info("Saved pixel difference: " + out_fp);
      delete pre;
      delete post;
    };

    // Post-filtering screenshot
    if (state.bilateral_filtering.pixel_diff) {
      if (state.bilateral_filtering.ambient)   { save_pixel_diff("ambient-radiance",  ambient_radiance_pixels, get_texture(gl_ambient_radiance_texture), TextureFormat::R32F); };
      if (state.bilateral_filtering.indirect)  { save_pixel_diff("indirect-radiance", indirect_radiance_pixels, get_texture(gl_indirect_radiance_texture)); };
      if (state.bilateral_filtering.specular)  { save_pixel_diff("specular-radiance", specular_radiance_pixels, get_texture(gl_specular_radiance_texture)); };
      state.bilateral_filtering.pixel_diff = false;
    }

    pass_ended();
  }

  /// Bilateral upsampling
  if (state.bilateral_upsample.enabled && state.lighting.downsample_modifier > 1) {
    assert(!state.bilinear_upsample.enabled);
    state.bilateral_upsample.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Bilateral upsampling pass");

    // NOTE: Joint bilateral upsampling of a filtered low-res noisy signal with high-res 'robust' signal(s)
    const auto bilateral_upsampling_pass = [&](const uint32_t in_texture, const uint32_t in_texture_unit) {
      // Ping - upsamples
      {
        const auto program = bs_ping_shader->gl_program;
        glUseProgram(program);
        glBindFramebuffer(GL_FRAMEBUFFER, gl_bs_ping_fbo);

        glTextureParameteri(in_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(in_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTextureParameteri(gbuffer_downsampled.gl_normal_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(gbuffer_downsampled.gl_normal_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTextureParameteri(gbuffer_downsampled.gl_position_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(gbuffer_downsampled.gl_position_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTextureParameteri(gbuffer_downsampled.gl_depth_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(gbuffer_downsampled.gl_depth_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glUniform1i(glGetUniformLocation(program, "uPosition_weight"), state.bilateral_upsample.position_weight);
        glUniform1i(glGetUniformLocation(program, "uPosition_high_res"), gl_position_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uPosition_low_res"), gbuffer_downsampled.gl_position_texture_unit);

        glUniform1i(glGetUniformLocation(program, "uNormal_weight"), state.bilateral_upsample.normal_weight);
        glUniform1i(glGetUniformLocation(program, "uNormal_high_res"), gl_geometric_normal_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uNormal_low_res"), gbuffer_downsampled.gl_normal_texture_unit);

        glUniform1i(glGetUniformLocation(program, "uTangent"), gl_tangent_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gl_tangent_normal_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uNormal_mapping"), state.bilateral_upsample.normal_mapping);

        glUniform1i(glGetUniformLocation(program, "uDepth_weight"), state.bilateral_upsample.depth_weight);
        glUniform1i(glGetUniformLocation(program, "uDepth_high_res"), gl_depth_texture_unit);
        glUniform1i(glGetUniformLocation(program, "uDepth_low_res"), gbuffer_downsampled.gl_depth_texture_unit);

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
    };

    // Declare input & output texture
    if (state.bilateral_upsample.ambient)  { bilateral_upsampling_pass(gl_ambient_radiance_texture,  gl_ambient_radiance_texture_unit);  }
    if (state.bilateral_upsample.indirect) { bilateral_upsampling_pass(gl_indirect_radiance_texture, gl_indirect_radiance_texture_unit); }
    if (state.bilateral_upsample.specular) { bilateral_upsampling_pass(gl_specular_radiance_texture, gl_specular_radiance_texture_unit); }

    pass_ended();
  }

  /// Bilinear upsampling
  if (state.bilinear_upsample.enabled && state.lighting.downsample_modifier > 1) {
    assert(!state.bilateral_upsample.enabled);
    state.bilinear_upsample.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Bilinear upsampling pass");

    // FIXME: Reuses some of Bilateral filtering ping-pong texture resources
    const auto bilinear_upsample = [&](const uint32_t in_texture, const uint32_t in_texture_unit) {
      const auto program = bilinear_upsampling_shader->gl_program;
      glUseProgram(program);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_bilinear_upsampling_fbo);

      if (state.bilinear_upsample.nearest_neighbor) {
        glTextureParameteri(in_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(in_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(gl_bf_ping_out_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(gl_bf_ping_out_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      } else {
        // Bilinear upsampling
        glTextureParameteri(in_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(in_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(gl_bf_ping_out_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(gl_bf_ping_out_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }

      // Ping
      {
        const float d = state.lighting.downsample_modifier;
        const Vec2f output_pixel_size = Vec2f(1.0f / (screen.width * d), 1.0f / (screen.height * d));
        glUniform2fv(glGetUniformLocation(program, "uOutput_pixel_size"), 1, &output_pixel_size.x);

        glUniform1i(glGetUniformLocation(program, "uInput"), in_texture_unit);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_bf_ping_out_texture, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      }

      // Pong - copies the color attacment of ping's FBO
      glCopyTextureSubImage2D(in_texture, 0, 0, 0, 0, 0, screen.width, screen.height);
    };

    if (state.bilinear_upsample.ambient)  { bilinear_upsample(gl_ambient_radiance_texture, gl_ambient_radiance_texture_unit);   }
    if (state.bilinear_upsample.indirect) { bilinear_upsample(gl_indirect_radiance_texture, gl_indirect_radiance_texture_unit); }
    if (state.bilinear_upsample.specular) { bilinear_upsample(gl_specular_radiance_texture, gl_specular_radiance_texture_unit); }

    last_executed_fbo = gl_bilinear_upsampling_fbo;

    pass_ended();
  }

  /// Lighting application pass
  {
    state.lighting.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Lighting application pass");

    const auto program = lighting_application_shader->gl_program;
    glUseProgram(program);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_lighting_application_fbo);

    const Vec2f pixel_size = Vec2(1.0f / screen.width, 1.0f / screen.height);
    glUniform2fv(glGetUniformLocation(program, "uPixel_size"), 1, &pixel_size.x);

    glUniform1i(glGetUniformLocation(program, "uDiffuse"), gl_diffuse_texture_unit);

    glUniform1i(glGetUniformLocation(program, "uIndirect_radiance"), gl_indirect_radiance_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uAmbient_radiance"), gl_ambient_radiance_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uSpecular_radiance"), gl_specular_radiance_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uEmissive_radiance"), gl_emissive_texture_unit);
    glUniform1i(glGetUniformLocation(program, "uDirect_radiance"), gl_direct_radiance_texture_unit);

    // Applicable radiance values
    const bool ambient_radiance_applicable = state.lighting.downsample_modifier > 1  ? (state.bilateral_upsample.enabled && state.bilateral_upsample.ambient) || (state.bilinear_upsample.enabled && state.bilinear_upsample.ambient) : true;
    glUniform1i(glGetUniformLocation(program, "uAmbient_radiance_applicable"), ambient_radiance_applicable);
    const bool indirect_radiance_applicable = state.lighting.downsample_modifier > 1  ? (state.bilateral_upsample.enabled && state.bilateral_upsample.indirect) || (state.bilinear_upsample.enabled && state.bilinear_upsample.indirect) : true;
    glUniform1i(glGetUniformLocation(program, "uIndirect_radiance_applicable"), indirect_radiance_applicable);
    const bool specular_radiance_applicable = state.lighting.downsample_modifier > 1  ? (state.bilateral_upsample.enabled && state.bilateral_upsample.specular) || (state.bilinear_upsample.enabled && state.bilinear_upsample.specular) : true;
    glUniform1i(glGetUniformLocation(program, "uSpecular_radiance_applicable"), specular_radiance_applicable);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    last_executed_fbo = gl_lighting_application_fbo;

    pass_ended();
  }

  /// Copy final pass into default FBO
  {
    state.blit.execution_time[TIME_IDX] = gl_query_time_buffer_ptr[state.render_passes];
    pass_started("Final blit pass");

    glBindFramebuffer(GL_READ_FRAMEBUFFER, last_executed_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    const auto mask = GL_COLOR_BUFFER_BIT;
    const auto filter = GL_NEAREST;
    glBlitFramebuffer(0, 0, screen.width, screen.height, 0, 0, screen.width, screen.height, mask, filter);

    pass_ended();
  }

  if (syncs[state.frame % 3]) glDeleteSync(syncs[state.frame % 3]);
  syncs[state.frame % 3] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

  #ifdef DEBUG
    log_gl_error();
  #endif

  state.graphic_batches = graphics_batches.size();

  if (state.capture_execution_timings) {
    /// Query for the GPU execution time for the render passes
    glBindBuffer(GL_QUERY_BUFFER, gl_execution_time_query_buffer);
    for (size_t i = 0; i < state.render_passes; i++) {
      glGetQueryObjectui64v(gl_query_ids[i], GL_QUERY_RESULT_NO_WAIT, reinterpret_cast<GLuint64*>(sizeof(GLuint64) * i));
      state.total_execution_time += gl_query_time_buffer_ptr[i]; // NOTE: Not 100% exact due to timings and changing pipeline
    }
  }
}

void Renderer::link_batch(GraphicsBatch& batch) {
  /// Geometry pass setup
  {
    /// Shaderbindings
    const auto program = batch.depth_shader.gl_program;
    glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "diffuse"), batch.gl_diffuse_texture_unit);
    glUniform1i(glGetUniformLocation(program, "pbr_parameters"), batch.gl_metallic_roughness_texture_unit);
    glUniform1i(glGetUniformLocation(program, "emissive"), batch.gl_emissive_texture_unit);
    glUniform1i(glGetUniformLocation(program, "tangent_normal"), batch.gl_tangent_normal_texture_unit);

    glGenVertexArrays(1, &batch.gl_depth_vao);
    glBindVertexArray(batch.gl_depth_vao);

    glGenBuffers(1, &batch.gl_depth_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_depth_vbo);
    glBufferData(GL_ARRAY_BUFFER, batch.mesh->byte_size_of_vertices(), batch.mesh->vertices.data(), GL_STATIC_DRAW);
    glObjectLabel(GL_BUFFER, batch.gl_depth_vbo, -1, "Batch gl_depth_vbo");

    // Element buffer
    glGenBuffers(1, &batch.gl_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.gl_ebo);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, batch.mesh->byte_size_of_indices(), batch.mesh->indices.data(), 0);
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
    batch.gl_material_buffer_ptr = (Material*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, GraphicsBatch::INIT_BUFFER_SIZE * sizeof(Material), flags);
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
  Material material;

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
      return;
    }

    if (comp.diffuse_texture.data.bytes_per_pixel == 3) {
      comp_shader_config.insert(Shader::Defines::DiffuseRGB);
    } else {
      comp_shader_config.insert(Shader::Defines::DiffuseRGBA);
    }
  } else if (comp.diffuse_scalars != Vec3f::zero()) {
    comp_shader_config.insert(Shader::Defines::DiffuseScalars);
    material.diffuse_scalars = comp.diffuse_scalars;
  }

  if (comp.emissive_texture.data.pixels) {
    comp_shader_config.insert(Shader::Defines::Emissive);
  } else if (comp.emissive_scalars != Vec3f::zero()) {
    comp_shader_config.insert(Shader::Defines::EmissiveScalars);
    material.emissive_scalars = comp.emissive_scalars;
  }

  // TODO: Tangent normals state 
  // if (comp.normal_texture.data.pixels) {
  //   comp_shader_config.insert(Shader::Defines:Emissive:TangentNormals);
  // }

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
        const bool is_sRGB = true; // FIXME: Assumes that diffuse textures are sRGB due to glTF2 material model

        /// Expand texture buffer if needed
        if (batch.diffuse_textures_count + 1 > batch.diffuse_textures_capacity) {
          batch.expand_texture_buffer(comp.diffuse_texture, &batch.gl_diffuse_texture_array, &batch.diffuse_textures_capacity, batch.gl_diffuse_texture_unit, is_sRGB);
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

  const uint32_t next_free_texture_unit = get_next_free_texture_unit(true);
  if (comp.diffuse_texture.data.pixels) {
    batch.gl_diffuse_texture_unit = next_free_texture_unit;

    const bool is_sRGB = true; // FIXME: Assumes that diffuse textures are in sRGB
    batch.init_buffer(comp.diffuse_texture, &batch.gl_diffuse_texture_array, batch.gl_diffuse_texture_unit, &batch.diffuse_textures_capacity, is_sRGB);

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(texture.gl_texture_target, 0, GL_RGB, texture.data.width, texture.data.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.data.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  if (comp.normal_texture.data.pixels) {
    const Texture& texture = comp.normal_texture;
    batch.gl_tangent_normal_texture_unit = next_free_texture_unit + 2;
    glActiveTexture(GL_TEXTURE0 + batch.gl_tangent_normal_texture_unit);
    glGenTextures(1, &batch.gl_tangent_normal_texture);
    glBindTexture(texture.gl_texture_target, batch.gl_tangent_normal_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(texture.gl_texture_target, 0, GL_RGB, texture.data.width, texture.data.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.data.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  if (comp.emissive_texture.data.pixels) {
    const Texture& texture = comp.emissive_texture;
    batch.gl_emissive_texture_unit = next_free_texture_unit + 3;
    glActiveTexture(GL_TEXTURE0 + batch.gl_emissive_texture_unit);
    glGenTextures(1, &batch.gl_emissive_texture);
    glBindTexture(texture.gl_texture_target, batch.gl_emissive_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(texture.gl_texture_target, 0, GL_RGB, texture.data.width, texture.data.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.data.pixels);
  }

  link_batch(batch);

  add_graphics_state(batch, comp, material, entity_id);
  graphics_batches.emplace_back(std::move(batch));
}

void Renderer::remove_component(const ID eid) {
  for (auto& batch : graphics_batches) {
    for (auto& id : batch.entity_ids) {
      if (id == eid) {
        // FIXME: Remove the Entity rendercomponent data from the objects struct in GBatch as well
        // FIXME: Delegate to the GBatch perhaps makes more sense?
        std::swap(id, batch.entity_ids.back());
        batch.entity_ids.pop_back();

        if (batch.entity_ids.empty()) {
          // TODO: Remove an empty GraphicsBatch ...
          // std::swap(batch, graphics_batches.back());
          graphics_batches.pop_back();
        }

        return;
      }
    }
  }
}

void Renderer::add_graphics_state(GraphicsBatch& batch, const RenderComponent& comp, Material material, ID entity_id) {
  if (batch.entity_ids.size() + 1 >= batch.buffer_size) {
    Log::warn("GraphicsBatch MAX_OBJECTS REACHED");
    batch.increase_entity_buffers(10);
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
  dest = (uint8_t*) &batch.gl_material_buffer_ptr[batch.objects.materials.size() - 1];
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

/// Pixels starts at the lower left corner then row major order
Vec3f* Renderer::take_screenshot(const uint32_t gl_fbo) const {
  glPixelStorei(GL_PACK_ROW_LENGTH, 0);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_PACK_SKIP_ROWS, 0);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_fbo);
  Vec3f* pixels = (Vec3f*) calloc(1, sizeof(Vec3f) * screen.width * screen.height);
  glReadPixels(0, 0, screen.width, screen.height, GL_RGB, GL_FLOAT, &pixels[0].x);
  return pixels;
}
