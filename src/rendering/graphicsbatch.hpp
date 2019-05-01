#pragma once
#ifndef MEINEKRAFT_GRAPHICSBATCH_HPP
#define MEINEKRAFT_GRAPHICSBATCH_HPP

#include <map>

#include "rendercomponent.hpp"
#include "../nodes/transform.hpp"
#include "primitives.hpp"
#include "shader.hpp"
#include "debug_opengl.hpp"
#include "meshmanager.hpp"

#ifdef _WIN32
#include <glew.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#else
#include <GL/glew.h>
#include <SDL2/SDL_image.h>
#include "sdl2/SDL_opengl.h"
#endif 

/// Bounding volume in the shape of a sphere
struct BoundingVolume {
  Vec3f position;      // Center position of the sphere         
  float radius = 1.0f; // Calculated somehow somewhere 
};

/// Material 
struct Material {
  uint32_t diffuse_layer_idx  = 0;
  ShadingModel shading_model  = ShadingModel::Unlit; // uint32_t
  Vec2f pbr_scalar_parameters = {}; // (roughness, metallic)
};

/// Computes the largest sphere radius fully containing the mesh [Ritter's algorithm]
static float compute_bounding_volume_radius(const Mesh& mesh) {
  // Compute extreme values along the axis
  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float min_z = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::min();
  float max_y = std::numeric_limits<float>::min();
  float max_z = std::numeric_limits<float>::min();
  std::array<Vec3f, 6> extremes = {}; // (minx, miny, minz, maxx, maxy, maxz)
  for (const Vertex& vert : mesh.vertices) {
    if (vert.position.x < min_x) { min_x = vert.position.x; extremes[0] = vert.position; }
    if (vert.position.y < min_y) { min_y = vert.position.y; extremes[1] = vert.position; }
    if (vert.position.z < min_z) { min_z = vert.position.z; extremes[2] = vert.position; }
    if (vert.position.x > max_x) { max_x = vert.position.x; extremes[3] = vert.position; }
    if (vert.position.y > max_x) { max_y = vert.position.y; extremes[4] = vert.position; }
    if (vert.position.z > max_x) { max_z = vert.position.z; extremes[5] = vert.position; }
  }

  // Find pair with the maximum distance between them
  float max_distance = std::numeric_limits<float>::min();
  std::array<Vec3f, 2> initial_sphere_points = {};
  for (const Vec3f& vert0 : extremes) {
    for (const Vec3f& vert1 : extremes) {
      if (vert0 == vert1) { continue; }
      const float distance = (vert0 - vert1).sqr_length();
      if (distance > max_distance) { 
        max_distance = distance; 
        initial_sphere_points[0] = vert0;
        initial_sphere_points[1] = vert1;
      }
    }
  }

  // Place sphere at the midpoint between them with radius as the distance between them
  BoundingVolume sphere;
  sphere.position = (initial_sphere_points[0] + initial_sphere_points[1]) / 2.0;
  sphere.radius = (initial_sphere_points[0] - initial_sphere_points[1]).length();

  // Adjust initial sphere in order to cover all vertices
  for (const Vertex& vert : mesh.vertices) {
    const float d = (sphere.position - vert.position).length();
    if (d > sphere.radius) {
      sphere.position = sphere.position + (d - sphere.radius) / 2.0f;
      sphere.radius = (d + sphere.radius) / 2.0f;
    }
  }
  Log::info("Bounding Volume sphere position: " + sphere.position.to_string());
  return sphere.radius;
}

struct GraphicsBatch {
  explicit GraphicsBatch(const ID mesh_id): mesh_id(mesh_id), objects{}, mesh{MeshManager::mesh_from_id(mesh_id)}, 
    layer_idxs{}, bounding_volume_radius(compute_bounding_volume_radius(mesh)) {};
  
  void init_buffer(const Texture& texture, uint32_t* gl_buffer, const uint32_t gl_texture_unit, uint32_t* buffer_capacity) {
    glActiveTexture(GL_TEXTURE0 + gl_texture_unit);
    glGenTextures(1, gl_buffer);
    glBindTexture(texture.gl_texture_target, *gl_buffer);
    const int default_buffer_size = 1;
    const GLuint texture_format = texture.data.bytes_per_pixel == 3 ? GL_RGB8 : GL_RGBA8;
    glTexStorage3D(texture.gl_texture_target, 1, texture_format, texture.data.width, texture.data.height, texture.data.faces * default_buffer_size); // depth = layer faces
    *buffer_capacity = default_buffer_size;
  }

  /// Increases the texture buffer and copies over the old texture buffer (this seems to be the only way to do it)
  void expand_texture_buffer(const Texture& texture, uint32_t* gl_buffer, uint32_t* texture_array_capacity, const uint32_t texture_unit) {
    // Allocate new memory
    uint32_t gl_new_texture_array;
    glGenTextures(1, &gl_new_texture_array);
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(texture.gl_texture_target, gl_new_texture_array);
    uint32_t old_capacity = *texture_array_capacity;
    *texture_array_capacity = (uint32_t) std::ceil(*texture_array_capacity * 1.5f);
    const GLuint texture_format = texture.data.bytes_per_pixel == 3 ? GL_RGB8 : GL_RGBA8;
    glTexStorage3D(texture.gl_texture_target, 1, texture_format, texture.data.width, texture.data.height, texture.data.faces * *texture_array_capacity);
    
    glCopyImageSubData(*gl_buffer, texture.gl_texture_target, 0, 0, 0, 0, // src parameters
      gl_new_texture_array, texture.gl_texture_target, 0, 0, 0, 0, texture.data.width, texture.data.height, texture.data.faces * old_capacity);

    // Update state
    glDeleteTextures(1, gl_buffer);
    *gl_buffer = gl_new_texture_array;
  }
  
  /// Upload a texture to the diffuse array
  void upload(const Texture& texture, const uint32_t gl_texture_unit, const uint32_t gl_texture_array) {
    const GLuint texture_format = texture.data.bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;
    glActiveTexture(GL_TEXTURE0 + gl_texture_unit);
    glBindTexture(texture.gl_texture_target, gl_texture_array);
    glTexSubImage3D(texture.gl_texture_target,
      0,                     // Mipmap number (a.k.a level)
      0, 0, layer_idxs[texture.id] *  texture.data.faces, // xoffset, yoffset, zoffset = layer face
      texture.data.width, texture.data.height, texture.data.faces, // width, height, depth = faces
      texture_format,        // format
      GL_UNSIGNED_BYTE,      // type
      texture.data.pixels);  // pointer to data
  }

  void increase_entity_buffers() {
    // FOR EACH BUFFER
    // 1. Create new larger buffer
    // 2. Map it
    // 3. Copy the old data
    // 4. Invalidate the old buffer
    // 5. Delete the old buffer object
    // 6. Update the GraphicsBatch state 
    glBindVertexArray(gl_depth_vao); // TODO: Something in this function seems to affect VAO state, which?
    const auto flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
    const uint32_t new_buffer_size = std::ceil(buffer_size * 2.0f);

    // Bounding volume buffer
    uint32_t new_gl_bounding_volume_buffer = 0;
    glGenBuffers(1, &new_gl_bounding_volume_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, new_gl_bounding_volume_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, new_buffer_size * sizeof(BoundingVolume), nullptr, flags);
    gl_bounding_volume_buffer_ptr = (uint8_t*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, new_buffer_size * sizeof(BoundingVolume), flags);
    glCopyNamedBufferSubData(gl_bounding_volume_buffer, new_gl_bounding_volume_buffer, 0, 0, buffer_size * sizeof(BoundingVolume));
    glInvalidateBufferData(gl_bounding_volume_buffer);
    glDeleteBuffers(1, &gl_bounding_volume_buffer);

    // Buffer for all the model matrices
    uint32_t new_gl_depth_models_buffer_object = 0;
    glGenBuffers(1, &new_gl_depth_models_buffer_object);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, new_gl_depth_models_buffer_object);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, new_buffer_size * sizeof(Mat4f), nullptr, flags);
    gl_depth_model_buffer_object_ptr = (uint8_t*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, new_buffer_size * sizeof(Mat4f), flags);
    glCopyNamedBufferSubData(gl_depth_models_buffer_object, new_gl_depth_models_buffer_object, 0, 0, buffer_size * sizeof(Mat4f));
    glInvalidateBufferData(gl_depth_models_buffer_object);
    glDeleteBuffers(1, &gl_depth_models_buffer_object);
    
    // Material buffer
    uint32_t new_gl_mbo = 0;
    glGenBuffers(1, &new_gl_mbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, new_gl_mbo);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, new_buffer_size * sizeof(Material), nullptr, flags);
    gl_mbo_ptr = (uint8_t*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, new_buffer_size * sizeof(Material), flags);
    glCopyNamedBufferSubData(gl_mbo, new_gl_mbo, 0, 0, buffer_size * sizeof(Material));
    glInvalidateBufferData(gl_mbo);
    glDeleteBuffers(1, &gl_mbo);

    // Batch instance idx buffer
    uint32_t new_gl_instance_idx_buffer = 0;
    glGenBuffers(1, &new_gl_instance_idx_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, new_gl_instance_idx_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, new_buffer_size * sizeof(GLuint), nullptr, 0);
    glBindBuffer(GL_ARRAY_BUFFER, new_gl_instance_idx_buffer);
    glVertexAttribIPointer(glGetAttribLocation(depth_shader.gl_program, "instance_idx"), 1, GL_UNSIGNED_INT, sizeof(GLuint), nullptr);
    glEnableVertexAttribArray(glGetAttribLocation(depth_shader.gl_program, "instance_idx"));
    glVertexAttribDivisor(glGetAttribLocation(depth_shader.gl_program, "instance_idx"), 1);
    glCopyNamedBufferSubData(gl_instance_idx_buffer, new_gl_instance_idx_buffer, 0, 0, buffer_size);
    glInvalidateBufferData(gl_instance_idx_buffer);
    glDeleteBuffers(1, &gl_instance_idx_buffer);

    // Update state
    gl_bounding_volume_buffer = new_gl_bounding_volume_buffer;
    gl_mbo = new_gl_mbo;
    gl_depth_models_buffer_object = new_gl_depth_models_buffer_object;
    gl_instance_idx_buffer = new_gl_instance_idx_buffer;
    buffer_size = new_buffer_size;
  }

  ID mesh_id; 
  Mesh mesh; 
  struct {
    std::vector<Mat4f> transforms;
    std::vector<BoundingVolume> bounding_volumes;     // Bounding volumes (Spheres for now)
    std::vector<Material> materials;                  
  } objects;
  std::unordered_map<ID, ID> data_idx;                // Entity ID to data position in data (objects struct)
  std::vector<ID> entity_ids;                         // Entities in the batch
  
  /// Textures
  std::map<ID, uint32_t> layer_idxs;  // Texture ID to layer index mapping for all texture in batch

  /// Diffuse texture buffer
  uint32_t diffuse_textures_count    = 0; // # texture currently in the GL buffer
  uint32_t diffuse_textures_capacity = 0; // # textures the GL buffer can hold
  
  uint32_t gl_diffuse_texture_array = 0;  // OpenGL handle to the texture array buffer (GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY, etc)
  uint32_t gl_diffuse_texture_unit  = 0;
  
  /// Physically based rendering related
  uint32_t gl_metallic_roughness_texture_unit = 0;  // Metallic roughness texture buffer
  uint32_t gl_metallic_roughness_texture = 0;

  uint32_t gl_ambient_occlusion_texture_unit  = 0;  // Ambient occlusion map
  uint32_t gl_emissive_texture_unit           = 0;  // Emissive map
    
  /// General
  static const uint32_t INIT_BUFFER_SIZE = 100;  // In # of elements 
  uint32_t buffer_size = INIT_BUFFER_SIZE;       // Current buffer size (of ALL buffers) for the batch 

  uint32_t gl_ebo = 0;            // Elements b.o
  uint8_t* gl_ebo_ptr = nullptr;  // Ptr to mapped GL_ELEMENTS_ARRAY_BUFFER

  const uint8_t gl_ibo_count = 3; // Number of partition of the buffer
  uint32_t gl_ibo = 0;            // (Draw) Indirect b.o (holds all draw commands)
  uint8_t* gl_ibo_ptr = nullptr;  // Ptr to mapped GL_DRAW_INDIRECT_BUFFER
  uint32_t gl_curr_ibo_idx = 0;   // Currently used partition of the buffer 

  uint32_t gl_bounding_volume_buffer = 0;           // Bounding volume buffer
  uint8_t* gl_bounding_volume_buffer_ptr = nullptr; // Ptr to the mapped bounding volume buffer
  
  // FIXME: Bounding volume geometry is the same across the batch, share this geometry instead
  float bounding_volume_radius = 0.0f; // Computed based on the batch geometry at batch creation 

  uint32_t gl_instance_idx_buffer = 0; // Instance indices passed along the shader pipeline for fetching per instance data from various buffers

  uint32_t gl_mbo = 0;           // Material b.o
  uint8_t* gl_mbo_ptr = nullptr; // Ptr to mapped material buffer

  /// Depth pass variables
  uint32_t gl_depth_vao = 0;
  uint32_t gl_depth_vbo = 0;
  uint32_t gl_depth_models_buffer_object  = 0;  // Models b.o (holds all objects model matrices / transforms)
  uint8_t* gl_depth_model_buffer_object_ptr = nullptr; // Model b.o ptr

  Shader depth_shader;  // Shader used to render all the components in this batch

  /// Shadow mapping pass variables
  uint32_t gl_shadowmapping_vao = 0;
};

#endif // MEINEKRAFT_GRAPHICSBATCH_HPP
