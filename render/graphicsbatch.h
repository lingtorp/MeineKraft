#ifndef MEINEKRAFT_GRAPHICSBATCH_H
#define MEINEKRAFT_GRAPHICSBATCH_H

#include <map>
#include <GL/glew.h>
#include <SDL_log.h>
#include <iostream>
#include <SDL2/SDL_image.h>
#include "primitives.h"
#include "shader.h"
#include "texturemanager.h"
#include "SDL2/SDL_opengl.h"

class RenderComponent;

/**
* Contains the rendering context for a given set of geometry data
* RenderComponents are batched in to a GraphicsBatch based on this geometry data & shader config.
*/
// TODO: Docs
class GraphicsBatch {
public:
  void log_gl_error() {
    GLenum err = glGetError();
    switch(err) {
      case GL_INVALID_VALUE:
        std::cout << glewGetErrorString(err) << std::endl;
        SDL_Log("GL_INVALID_VALUE");
        break;
      default:
        if (err != 0) {
          std::cout << glewGetErrorString(err) << std::endl;
          SDL_Log("OpenGL error: %i", err);
        }
        break;
    }
  }
  
  explicit GraphicsBatch(ID mesh_id): mesh_id(mesh_id), components{}, mesh{}, gl_camera_view(0),
  gl_models_buffer_object(0), gl_VAO(0), id(0), diffuse_textures{}, layer_idxs{} {
    glGenTextures(1, &gl_diffuse_texture_array);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, gl_diffuse_texture_array);
    
    // For now create a existent buffer to hold cube maps
    auto layers_faces = 6 * 3; // Room for 2 cube maps
    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGB8, 512, 512, layers_faces);
    diffuse_textures_count = 0;
  };
  
  RawTexture load_textures(GraphicsState* g_state) {
    RawTexture texture{0, nullptr};
    auto& files = g_state->diffuse_texture.resource.files;
    
    auto& file = files.front();
    SDL_Surface* image = IMG_Load(file.c_str());
    texture.width  = static_cast<uint32_t>(image->w);
    texture.height = static_cast<uint32_t>(image->h);
    auto bytes_per_pixel = image->format->BytesPerPixel;
    SDL_Log("Bytes per pixel: %i", bytes_per_pixel);
    SDL_FreeSurface(image); // Wasteful
  
    // Assumes that the files are the same size, in right order, same encoding, etc
    texture.size = bytes_per_pixel * texture.width * texture.height;
    texture.data = static_cast<uint8_t *>(std::calloc(1, texture.size * files.size()));
    // Load all the files into a linear memory region
    for (size_t i = 0; i < files.size(); i++) {
      image = IMG_Load(files[i].c_str());
      std::memcpy(texture.data + texture.size * i, image->pixels, texture.size);
      SDL_FreeSurface(image);
    }
    
    return texture;
  }

  /// Id given to each unique mesh loaded by MeshManager
  ID mesh_id;
  Mesh mesh;
  
  /// Batch id
  ID id;
  
  /// Textures
  std::vector<ID> texture_ids;
  std::map<ID, uint32_t> layer_idxs;
  // Diffuse
  std::vector<ID> diffuse_textures;
  uint32_t diffuse_textures_count;
  uint32_t gl_diffuse_texture_array;
  uint32_t gl_diffuse_texture_unit = GL_TEXTURE0;
  
  std::vector<RenderComponent*> components;

  uint32_t gl_VAO;
  uint32_t gl_models_buffer_object;
  uint32_t gl_camera_view;
  uint32_t gl_camera_position;
  uint32_t gl_diffuse_texture_layer_idx; // sampler layer index buffer

  Shader shader;
};

#endif // MEINEKRAFT_GRAPHICSBATCH_H
