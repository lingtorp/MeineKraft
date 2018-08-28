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
#include <sdl2/SDL_image.h>
#include "sdl2/SDL_opengl.h"
#endif 

class RenderComponent;

class GraphicsBatch {
public:
  explicit GraphicsBatch(ID mesh_id): mesh_id(mesh_id), components{}, mesh{}, id(0), diffuse_textures{}, layer_idxs{},
    diffuse_textures_capacity(3), diffuse_textures_count(0) {};
  
  void init_buffer(uint32_t* gl_buffer, uint32_t gl_texture_unit, const Texture& texture) {
    glGenTextures(1, gl_buffer); 
    glActiveTexture(GL_TEXTURE0 + gl_texture_unit);
    glBindTexture(texture.gl_texture_type, *gl_buffer);
    const int buffer_size = 3;
    glTexStorage3D(texture.gl_texture_type, buffer_size, GL_RGB8, texture.data.width, texture.data.height, 1);
  }

  /// GL buffer type or in GL-speak target rather than type
  void expand_texture_buffer(uint32_t* gl_buffer, uint32_t gl_buffer_type) {
    // TODO: Not texture aware
    
    /// Allocate new memory
    uint32_t gl_new_texture_array;
    glGenTextures(1, &gl_new_texture_array);
    glActiveTexture(GL_TEXTURE0 + gl_diffuse_texture_unit);
    glBindTexture(gl_buffer_type, gl_new_texture_array);
    
    // # of new textures to accommodate
    auto new_textures_capacity = (uint32_t) std::ceil(diffuse_textures_capacity * texture_array_growth_factor);
    auto layers_faces = 6 * new_textures_capacity;
    glTexStorage3D(gl_buffer_type, 1, GL_RGB8, 512, 512, layers_faces);
    
    /// Update state
    *gl_buffer = gl_new_texture_array;
    diffuse_textures_capacity = new_textures_capacity;
  }
  
  /// Upload a texture to the diffuse array
  void upload(Texture texture) {
    glActiveTexture(gl_diffuse_texture_unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, gl_diffuse_texture_array);
    glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,
      0,                     // Mipmap number
      0, 0, texture.layer_idx * 6, // xoffset, yoffset, zoffset = layer face
      512, 512, 6,           // width, height, depth = faces
      GL_RGB,                // format
      GL_UNSIGNED_BYTE,      // type
      texture.data.pixels);  // pointer to data
  }

  /// Id given to each unique mesh loaded by MeshManager
  ID mesh_id;
  Mesh mesh;
  
  /// Batch id
  ID id;
  
  /// Textures
  std::map<ID, uint32_t> layer_idxs; // Texture ID to layer index mapping

  const float texture_array_growth_factor = 1.5f; // new_buf_size = ceil(old_buf_size * growth_factor)

  // Diffuse texture buffer
  std::vector<ID> diffuse_textures;
  uint32_t diffuse_textures_count;    // # texture currently in the GL buffer
  uint32_t diffuse_textures_capacity; // # textures the GL buffer can hold
  
  uint32_t gl_diffuse_texture_array;  // OpenGL handle to the texture array buffer
  uint32_t gl_diffuse_texture_type;   // CUBE_MAP_ARRAY, 2D_TEXTURE_ARRAY, etc
  uint32_t gl_diffuse_texture_unit = 11; // FIXME: Hard coded
  
  uint32_t gl_diffuse_textures_layer_idx;
  
  std::vector<RenderComponent*> components;
  
  /// Depth pass variables
  uint32_t gl_depth_vao;
  uint32_t gl_depth_vbo;
  uint32_t gl_depth_models_buffer_object;
};

#endif // MEINEKRAFT_GRAPHICSBATCH_H
