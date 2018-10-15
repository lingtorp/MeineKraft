#ifndef MEINEKRAFT_GRAPHICSBATCH_H
#define MEINEKRAFT_GRAPHICSBATCH_H

#include <map>
#include <cstring>

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
  explicit GraphicsBatch(ID mesh_id): mesh_id(mesh_id), objects{}, mesh{}, layer_idxs{},
    diffuse_textures_capacity(5), diffuse_textures_count(0) {};
  
  void init_buffer(uint32_t* gl_buffer, const uint32_t gl_texture_unit, const Texture& texture) {
    glActiveTexture(GL_TEXTURE0 + gl_texture_unit);
    glGenTextures(1, gl_buffer);
    glBindTexture(texture.gl_texture_target, *gl_buffer);
    const int default_buffer_size = 3;
    glTexStorage3D(texture.gl_texture_target, 1, GL_RGB8, texture.data.width, texture.data.height, texture.data.faces * default_buffer_size); // depth = layer faces
  }

  /// GL buffer type or in GL-speak target rather than type
  void expand_texture_buffer(uint32_t* gl_buffer, const Texture& texture) {
    // TODO: Dealloc the old buffer
    // Allocate new memory
    uint32_t gl_new_texture_array;
    glGenTextures(1, &gl_new_texture_array);
    glActiveTexture(GL_TEXTURE0 + gl_diffuse_texture_unit);
    glBindTexture(texture.gl_texture_target, gl_new_texture_array);
    
    // # of new textures to accommodate
    const float texture_array_growth_factor = 1.5f; 
    diffuse_textures_capacity = (uint32_t) std::ceil(diffuse_textures_capacity * texture_array_growth_factor);
    glTexStorage3D(texture.gl_texture_target, 1, GL_RGB8, texture.data.width, texture.data.height, texture.data.faces * diffuse_textures_capacity);
    
    // Update state
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

  void add_graphics_state(const RenderComponent& comp, const ID entity_id) {
    entity_ids.push_back(entity_id);
    objects.diffuse_textures.push_back(comp.diffuse_texture);
    objects.emissive_textures.push_back(comp.emissive_texture);
    objects.metallic_roughness_textures.push_back(comp.metallic_roughness_texture);
    objects.ambient_occlusion_textures.push_back(comp.ambient_occlusion_texture);
    objects.pbr_scalar_parameters.push_back(comp.pbr_scalar_parameters);
    objects.shading_models.push_back(comp.shading_model);
    num_objects++;
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
  std::vector<ID> entity_ids;
  GraphicStateObjects objects{}; // Objects in the batch share the same values
  uint64_t num_objects = 0; 
  
  /// Textures
  std::map<ID, uint32_t> layer_idxs; // Texture ID to layer index mapping for all texture in batch

  /// Diffuse texture buffer
  uint32_t diffuse_textures_count;    // # texture currently in the GL buffer
  uint32_t diffuse_textures_capacity; // # textures the GL buffer can hold
  
  uint32_t gl_diffuse_texture_array;  // OpenGL handle to the texture array buffer (GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY, etc)
  uint32_t gl_diffuse_texture_type;   // GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc
  uint32_t gl_diffuse_texture_unit;
  
  uint32_t gl_diffuse_textures_layer_idx; // Attribute buffer for layer indices
  
  /// Physically based rendering related
  uint32_t gl_metallic_roughness_texture_unit;  // Metallic roughness texture buffer
  uint32_t gl_ambient_occlusion_texture_unit;   // Ambient occlusion map
  uint32_t gl_emissive_texture_unit;            // Emissive map
    
  /// Depth pass variables
  uint32_t gl_depth_vao;
  uint32_t gl_depth_vbo;
  uint32_t gl_depth_models_buffer_object;
  uint32_t gl_shading_model_buffer_object;
  uint32_t gl_pbr_scalar_buffer_object;         // Used by ShadingModel::PBRScalars

  ShadingModel shading_model; // Shading model to use with the shader
  Shader depth_shader;        // Shader used to render all the components in this batch
};

#endif // MEINEKRAFT_GRAPHICSBATCH_H
