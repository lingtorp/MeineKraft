#ifndef MEINEKRAFT_GRAPHICSBATCH_H
#define MEINEKRAFT_GRAPHICSBATCH_H

#include <map>
#include <cstring>

#include "rendercomponent.h"
#include "../nodes/transform.h"
#include "primitives.h"
#include "shader.h"
#include "debug_opengl.h"

#ifdef _WIN32
#include <glew.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#else
#include <GL/glew.h>
#include <SDL2/SDL_image.h>
#include "sdl2/SDL_opengl.h"
#endif 

class GraphicsBatch {
public:
  explicit GraphicsBatch(ID mesh_id): mesh_id(mesh_id), objects{}, mesh{}, layer_idxs{} {};
  
  void init_buffer(const Texture& texture, uint32_t* gl_buffer, const uint32_t gl_texture_unit, uint32_t* buffer_capacity) {
    glActiveTexture(GL_TEXTURE0 + gl_texture_unit);
    glGenTextures(1, gl_buffer);
    glBindTexture(texture.gl_texture_target, *gl_buffer);
    const int default_buffer_size = 1;
    glTexStorage3D(texture.gl_texture_target, 1, GL_RGB8, texture.data.width, texture.data.height, texture.data.faces * default_buffer_size); // depth = layer faces
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
    glTexStorage3D(texture.gl_texture_target, 1, GL_RGB8, texture.data.width, texture.data.height, texture.data.faces * *texture_array_capacity);
    
    glCopyImageSubData(*gl_buffer, texture.gl_texture_target, 0, 0, 0, 0, // src parameters
      gl_new_texture_array, texture.gl_texture_target, 0, 0, 0, 0, texture.data.width, texture.data.height, texture.data.faces * old_capacity);

    // Update state
    glDeleteTextures(1, gl_buffer);
    *gl_buffer = gl_new_texture_array;
  }
  
  /// Upload a texture to the diffuse array
  void upload(const Texture& texture, const uint32_t gl_texture_unit, const uint32_t gl_texture_array) {
    glActiveTexture(GL_TEXTURE0 + gl_texture_unit);
    glBindTexture(texture.gl_texture_target, gl_texture_array);
    glTexSubImage3D(texture.gl_texture_target,
      0,                     // Mipmap number (a.k.a level)
      0, 0, layer_idxs[texture.id] *  texture.data.faces, // xoffset, yoffset, zoffset = layer face
      texture.data.width, texture.data.height, texture.data.faces, // width, height, depth = faces
      GL_RGB,                // format
      GL_UNSIGNED_BYTE,      // type
      texture.data.pixels);  // pointer to data
  }

  ID mesh_id; 
  Mesh mesh; 
  struct GraphicStateObjects {
    std::vector<Transform> transforms;
    std::vector<ShadingModel> shading_models;         // default ShadingModel::Unlit;
    std::vector<Texture> diffuse_textures;
    std::vector<uint32_t> diffuse_texture_idxs;       // Layer index
    std::vector<Texture> metallic_roughness_textures; // Used by ShadingModel::PBRTextured
    std::vector<Texture> ambient_occlusion_textures;
    std::vector<Texture> emissive_textures;
    std::vector<Vec3f> pbr_scalar_parameters;         // Used by ShadingModel::PBRScalars
  };
  std::unordered_map<ID, ID> data_idx;                // Entity ID to data position in data (objects struct)
  std::vector<ID> entity_ids;
  GraphicStateObjects objects{};                      // Objects in the batch share the same values
  
  /// Textures
  std::map<ID, uint32_t> layer_idxs;  // Texture ID to layer index mapping for all texture in batch

  /// Diffuse texture buffer
  uint32_t diffuse_textures_count    = 0; // # texture currently in the GL buffer
  uint32_t diffuse_textures_capacity = 0; // # textures the GL buffer can hold
  
  uint32_t gl_diffuse_texture_array = 0;  // OpenGL handle to the texture array buffer (GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY, etc)
  uint32_t gl_diffuse_texture_type  = 0;  // GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc
  uint32_t gl_diffuse_texture_unit  = 0;
  
  uint32_t gl_diffuse_textures_layer_idx = 0; // Attribute buffer for layer indices
  
  /// Physically based rendering related
  uint32_t gl_metallic_roughness_texture_unit = 0;  // Metallic roughness texture buffer
  uint32_t gl_ambient_occlusion_texture_unit  = 0;  // Ambient occlusion map
  uint32_t gl_emissive_texture_unit           = 0;  // Emissive map
    
  /// Depth pass variables
  uint32_t gl_depth_vao = 0;
  uint32_t gl_depth_vbo = 0;
  uint32_t gl_depth_models_buffer_object  = 0;
  uint32_t gl_shading_model_buffer_object = 0;
  uint32_t gl_pbr_scalar_buffer_object    = 0;         // Used by ShadingModel::PBRScalars

  ShadingModel shading_model; // Shading model to use with the shader
  Shader depth_shader;        // Shader used to render all the components in this batch
};

#endif // MEINEKRAFT_GRAPHICSBATCH_H
