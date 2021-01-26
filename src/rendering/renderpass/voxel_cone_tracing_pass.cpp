#include "voxel_cone_tracing_pass.hpp"

#include "../graphicsbatch.hpp"
#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../math/vector.hpp"
#include "../../rendering/primitives.hpp"
#include "../../util/filesystem.hpp"
#include "../../nodes/model.hpp"
#include "gbuffer_pass.hpp"

#include <vector>

#include <GL/glew.h>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

bool VoxelConeTracingRenderPass::setup(Renderer* render) {
  const RenderState state = render->state;
  const Resolution screen = render->screen;

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

  shader = new Shader(Filesystem::base + "shaders/voxel-cone-tracing.vert",
                      Filesystem::base + "shaders/voxel-cone-tracing.frag");

  const std::string includes = Filesystem::read_file(Filesystem::base + "shaders/voxel-cone-tracing-utils.glsl");
  shader->add(includes);

  const auto [ok, err_msg] = shader->compile();
  if (!ok) {
    Log::error("Voxel cone tracing shader error: " + err_msg);
    return false;
  }

  const auto program = shader->gl_program;
  glUseProgram(program);

  glGenFramebuffers(1, &gl_vct_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_vct_fbo);

  // Global indirect radiance map
  gl_indirect_radiance_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_indirect_radiance_texture_unit);
  glGenTextures(1, &gl_indirect_radiance_texture);
  glBindTexture(GL_TEXTURE_2D, gl_indirect_radiance_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_indirect_radiance_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_indirect_radiance_texture, -1, "GBuffer indirect radiance texture");

  // Global ambient radiance map
  gl_ambient_radiance_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_ambient_radiance_texture_unit);
  glGenTextures(1, &gl_ambient_radiance_texture);
  glBindTexture(GL_TEXTURE_2D, gl_ambient_radiance_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, screen.width, screen.height, 0, GL_RED, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gl_ambient_radiance_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_ambient_radiance_texture, -1, "GBuffer ambient radiance texture");

  // Global specular radiance map
  gl_specular_radiance_texture_unit = render->get_next_free_texture_unit();
  glActiveTexture(GL_TEXTURE0 + gl_specular_radiance_texture_unit);
  glGenTextures(1, &gl_specular_radiance_texture);
  glBindTexture(GL_TEXTURE_2D, gl_specular_radiance_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen.width, screen.height, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gl_specular_radiance_texture, 0);
  glObjectLabel(GL_TEXTURE, gl_specular_radiance_texture, -1, "GBuffer specular radiance texture");

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, gbuffer_pass->gl_direct_radiance_texture, 0);

  const uint32_t attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
  glDrawBuffers(std::size(attachments), attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::error("VCT fbo not complete"); log_gl_error();
    return false;
  }

  glGenVertexArrays(1, &gl_vct_vao);
  glBindVertexArray(gl_vct_vao);

  GLuint gl_vbo = 0;
  glGenBuffers(1, &gl_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Primitive::quad), &Primitive::quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
  glVertexAttribPointer(glGetAttribLocation(program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  return true;
}

bool VoxelConeTracingRenderPass::render(Renderer* render) {
  const Resolution screen = render->screen;
  const RenderState state = render->state;
  const auto NUM_CLIPMAPS = Renderer::NUM_CLIPMAPS;
  const Scene* scene = render->scene;

  render->pass_started("Voxel cone tracing pass");
  const auto program = shader->gl_program;

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
  const float metallic_aperature = glm::radians(state.vct.metallic_aperature);
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
    scaling_factors[i] = 1.0f / render->clipmaps.aabb[i].max_axis();
  }
  glUniform1fv(glGetUniformLocation(program, "uScaling_factors"), NUM_CLIPMAPS, scaling_factors);

  const float voxel_size_LOD0 = render->clipmaps.aabb[0].max_axis() / float(render->clipmaps.size[0]);
  glUniform1f(glGetUniformLocation(program, "uVoxel_size_LOD0"), voxel_size_LOD0);

  // TODO: Precompute these - add notification when changed and precompute the new ones
  Vec3f clipmap_aabb_centers[NUM_CLIPMAPS];
  Vec3f clipmap_aabb_mins[NUM_CLIPMAPS];
  Vec3f clipmap_aabb_maxs[NUM_CLIPMAPS];

  // TODO: Precompute these - add notification when changed and precompute the new ones
  for (size_t i = 0; i < NUM_CLIPMAPS; i++) {
    clipmap_aabb_centers[i] = render->clipmaps.aabb[i].center();
    clipmap_aabb_mins[i] = render->clipmaps.aabb[i].min;
    clipmap_aabb_maxs[i] = render->clipmaps.aabb[i].max;
  }

  glUniform3fv(glGetUniformLocation(program, "uAABB_centers"), NUM_CLIPMAPS, &clipmap_aabb_centers[0].x);
  glUniform3fv(glGetUniformLocation(program, "uAABB_mins"), NUM_CLIPMAPS, &clipmap_aabb_mins[0].x);
  glUniform3fv(glGetUniformLocation(program, "uAABB_maxs"), NUM_CLIPMAPS, &clipmap_aabb_maxs[0].x);
  glUniform1iv(glGetUniformLocation(program, "uVoxel_radiance"), NUM_CLIPMAPS, render->gl_voxel_radiance_texture_units);
  glUniform1iv(glGetUniformLocation(program, "uVoxel_opacity"), NUM_CLIPMAPS, render->gl_voxel_opacity_texture_units);
  glUniform1ui(glGetUniformLocation(program, "uScreen_width"), screen.width / state.lighting.downsample_modifier);
  glUniform1ui(glGetUniformLocation(program, "uScreen_height"), screen.height / state.lighting.downsample_modifier);
  glUniform1i(glGetUniformLocation(program, "uPosition"), gbuffer_pass->gl_position_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uNormal"), gbuffer_pass->gl_geometric_normal_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uPBR_parameters"), gbuffer_pass->gl_pbr_parameters_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uTangent_normal"), gbuffer_pass->gl_tangent_normal_texture_unit);
  glUniform1i(glGetUniformLocation(program, "uTangent"), gbuffer_pass->gl_tangent_texture_unit);

  glViewport(0, 0, screen.width / state.lighting.downsample_modifier, screen.height / state.lighting.downsample_modifier);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glViewport(0, 0, screen.width, screen.height);

  render->pass_ended();

  return false;
}
