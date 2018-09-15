#ifndef MEINEKRAFT_GRAPHICSBATCH_H
#define MEINEKRAFT_GRAPHICSBATCH_H

#include <map>
#include <SDL_log.h>
#include <iostream>
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

class RenderComponent;

class GraphicsBatch {
public:
  explicit GraphicsBatch(ID mesh_id): mesh_id(mesh_id), components{}, mesh{}, id(0), diffuse_textures{}, layer_idxs{},
    diffuse_textures_capacity(3), diffuse_textures_count(0) {};
  
  void init_buffer(uint32_t* gl_buffer, const uint32_t gl_texture_unit, const Texture& texture) {
    glGenTextures(1, gl_buffer); 
    glActiveTexture(GL_TEXTURE0 + gl_texture_unit);
    glBindTexture(texture.gl_texture_type, *gl_buffer);
    const int buffer_size = 3;
    glTexStorage3D(texture.gl_texture_type, 1, GL_RGB8, texture.data.width, texture.data.height, texture.data.faces * buffer_size); // depth = layer faces
    if (texture.gl_texture_type == GL_TEXTURE_CUBE_MAP) {
      glTexParameteri(texture.gl_texture_type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(texture.gl_texture_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(texture.gl_texture_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(texture.gl_texture_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(texture.gl_texture_type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
  }

  /// GL buffer type or in GL-speak target rather than type
  void expand_texture_buffer(uint32_t* gl_buffer, const uint32_t gl_buffer_type) {
    // TODO: NOT TEXTURE AWARE
    
    /// Allocate new memory
    uint32_t gl_new_texture_array;
    glGenTextures(1, &gl_new_texture_array);
    glActiveTexture(GL_TEXTURE0 + gl_diffuse_texture_unit);
    glBindTexture(gl_buffer_type, gl_new_texture_array);
    
    // # of new textures to accommodate
    const float texture_array_growth_factor = 1.5f; 
    auto new_textures_capacity = (uint32_t) std::ceil(diffuse_textures_capacity * texture_array_growth_factor);
    auto layers_faces = 6 * new_textures_capacity; 
    glTexStorage3D(gl_buffer_type, 1, GL_RGB8, 512, 512, layers_faces);
    
    /// Update state
    *gl_buffer = gl_new_texture_array;
    diffuse_textures_capacity = new_textures_capacity;
  }
  
  /// Upload a texture to the diffuse array
  void upload(const Texture& texture, const uint32_t gl_texture_unit) {
    glActiveTexture(GL_TEXTURE0 + gl_texture_unit);
    glBindTexture(texture.gl_texture_type, gl_diffuse_texture_array);
    glTexSubImage3D(texture.gl_texture_type,
      0,                     // Mipmap number (a.k.a level)
      0, 0, texture.layer_idx *  texture.data.faces, // xoffset, yoffset, zoffset = layer face
      texture.data.width, texture.data.height, texture.data.faces, // width, height, depth = faces
      GL_RGB,                // format
      GL_UNSIGNED_BYTE,      // type
      texture.data.pixels);  // pointer to data
  }

  ID id;
  ID mesh_id;
  Mesh mesh; 
  
  /// Textures
  std::map<ID, uint32_t> layer_idxs; // Texture ID to layer index mapping

  // Diffuse texture buffer
  std::vector<ID> diffuse_textures;
  uint32_t diffuse_textures_count;    // # texture currently in the GL buffer
  uint32_t diffuse_textures_capacity; // # textures the GL buffer can hold
  
  uint32_t gl_diffuse_texture_array;  // OpenGL handle to the texture array buffer (GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY, etc)
  uint32_t gl_diffuse_texture_type;   // GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc
  uint32_t gl_diffuse_texture_unit;
  
  uint32_t gl_diffuse_textures_layer_idx;
  
  // Metallic roughness texture buffer
  uint32_t gl_metallic_roughness_texture_unit;
  
  std::vector<RenderComponent*> components;
  
  /// Depth pass variables
  uint32_t gl_depth_vao;
  uint32_t gl_depth_vbo;
  uint32_t gl_depth_models_buffer_object;
};

#endif // MEINEKRAFT_GRAPHICSBATCH_H
