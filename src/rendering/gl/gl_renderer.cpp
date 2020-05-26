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

#include "renderpass/downsample_pass.hpp"
#include "renderpass/gbuffer_pass.hpp"
#include "renderpass/directionalshadow_pass.hpp"
#include "renderpass/lighting_application_pass.hpp"
#include "renderpass/direct_lighting_pass.hpp"
#include "renderpass/voxelization_pass.hpp"
#include "renderpass/voxel_cone_tracing_pass.hpp"
#include "renderpass/view_frustum_culling_pass.hpp"
#include "renderpass/bilinear_upsampling_pass.hpp"
#include "renderpass/bilateral_filtering_pass.hpp"
#include "renderpass/bilateral_upsampling_pass.hpp"

#include "camera.hpp"
#include "debug_opengl.hpp"
#include "graphicsbatch.hpp"
#include "meshmanager.hpp"
#include "rendercomponent.hpp"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/// Indicates the start of a Renderpass (must be paried with pass_ended);
inline void Renderer::pass_started(const std::string &name) {
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
  state.render_passes++;
}

/// Indicates the end of a Renderpass (must be paired with pass_started)
inline void Renderer::pass_ended() const {
  glPopDebugGroup();
}

uint32_t Renderer::get_next_free_texture_unit(bool peek) {
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

uint32_t Renderer::get_next_free_image_unit(bool peek) {
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

    // TODO: Environment map support
    Log::info("Image based lighting is not implemented");
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
glm::mat4 Renderer::orthographic_projection(const AABB& aabb) {
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
  // Rendergraph construction and setup
  gbuffer_pass = new GbufferRenderPass();
  downsample_pass = new DownsampleRenderPass();
  shadow_pass = new DirectionalShadowRenderPass();
  direct_lighting_pass = new DirectLightingRenderPass();
  lighting_application_pass = new LightingApplicationRenderPass();
  voxel_cone_tracing_pass = new VoxelConeTracingRenderPass();
  voxelization_pass = new VoxelizationRenderPass();
  view_frustum_culling_pass = new ViewFrustumCullingRenderPass();
  bilinear_upsampling_pass = new BilinearUpsamplingRenderPass();
  bilateral_filtering_pass = new BilateralFilteringRenderPass();
  bilateral_upsampling_pass = new BilateralUpsamplingRenderPass();

  voxelization_pass->shadow_pass = shadow_pass;
  voxelization_pass->gbuffer_pass = gbuffer_pass;

  voxel_cone_tracing_pass->gbuffer_pass = gbuffer_pass;

  downsample_pass->shadow_pass = shadow_pass;
  downsample_pass->gbuffer_pass = gbuffer_pass;

  lighting_application_pass->gbuffer_pass = gbuffer_pass;
  lighting_application_pass->voxel_cone_tracing_pass = voxel_cone_tracing_pass;

  direct_lighting_pass->gbuffer_pass = gbuffer_pass;
  direct_lighting_pass->shadow_pass  = shadow_pass;

  bilinear_upsampling_pass->voxel_cone_tracing_pass = voxel_cone_tracing_pass;

  bilateral_filtering_pass->gbuffer_pass = gbuffer_pass;

  bilateral_upsampling_pass->gbuffer_pass = gbuffer_pass;
  bilateral_upsampling_pass->voxel_cone_tracing_pass = voxel_cone_tracing_pass;
  bilateral_upsampling_pass->downsample_pass = downsample_pass;

  // NOTE: Order important for certain passes
  render_passes.push_back(gbuffer_pass);
  render_passes.push_back(downsample_pass);
  render_passes.push_back(shadow_pass);
  render_passes.push_back(lighting_application_pass);
  render_passes.push_back(direct_lighting_pass);
  render_passes.push_back(voxelization_pass);
  render_passes.push_back(voxel_cone_tracing_pass);
  render_passes.push_back(view_frustum_culling_pass);
  render_passes.push_back(bilinear_upsampling_pass);
  render_passes.push_back(bilateral_filtering_pass);
  render_passes.push_back(bilateral_upsampling_pass);

  for (auto render_pass : render_passes) {
    if (!render_pass->setup(this)) {
      Log::warn("RenderPass failed to init");
      // TODO: Better error logging.
    }
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

  // Asserts downsampling factor
  if (state.lighting.downsample_modifier % 2 != 0 || state.lighting.downsample_modifier <= 0) {
    state.lighting.downsample_modifier = 1;
  }

  const float aspect_ratio = static_cast<float>(screen.width) / static_cast<float>(screen.height);
  projection_matrix = scene->camera.projection(aspect_ratio);

  camera_transform = projection_matrix * scene->camera.transform(); // TODO: Camera handling needs to be reworked

  /// Renderer caches the transforms of components thus we need to fetch the ones who changed during the last frame
  // FIXME: This is a bad design?
  // update_transforms();
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

  if (state.culling.enabled) {
    view_frustum_culling_pass->render(this);
  }

  /// Directional shadowmapping
  shadow_pass->light_space_transform = shadowmap_transform(scene->aabb, scene->directional_light);
  shadow_pass->render(this);

  /// Gbuffer pass
  gbuffer_pass->render(this);

  /// Downsample pass
  downsample_pass->render(this);

  /// Voxelization pass
  if (state.voxelization.voxelize) {
    if (!state.voxelization.always_voxelize) {
      state.voxelization.voxelize = false;
    }

    voxelization_pass->render(this);
  }

  /// Voxel cone tracing pass
  voxel_cone_tracing_pass->render(this);

  /// Direct lighting (non-VCT) pass
  if (state.lighting.direct && state.shadow.algorithm != ShadowAlgorithm::VCT) {
    direct_lighting_pass->render(this);
  }

  /// Bilateral filtering pass
  if (state.bilateral_filtering.enabled) {
    bilateral_filtering_pass->render(this);
  }

  if (state.bilateral_upsample.enabled && state.lighting.downsample_modifier > 1) {
    assert(!state.bilinear_upsample.enabled);
    bilateral_upsampling_pass->render(this);
  }

  /// Bilinear upsampling pass
  bilinear_upsampling_pass->render(this);

  /// Lighting application pass
  lighting_application_pass->render(this);

  /// Copy final pass into default FBO
  {
    pass_started("Final blit pass");

    glBindFramebuffer(GL_READ_FRAMEBUFFER, lighting_application_pass->gl_lighting_application_fbo);
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
    const auto program = shadow_pass->shadowmapping_shader->gl_program;
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
    const auto program = voxelization_pass->shader->gl_program;
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
