#ifndef MEINEKRAFT_TEXTURE_H
#define MEINEKRAFT_TEXTURE_H

#include <cstdint>
#include <string>
#include <vector>

/// Opaque ID type used to reference resources throughout the engine
typedef uint64_t ID;

enum class FileExtension {
  png, jpg, unknown
};

struct TextureResource {
  TextureResource() = default;
  std::vector<std::string> files;
  
  explicit TextureResource(std::string file): files{file} {};
  
  uint64_t to_hash() const {
    uint64_t hash = 0;
    for (const auto& file : files) {
      hash += std::hash<std::string>{}(file);
    }
    return hash;
  }
};

struct Texture {
  Texture() = default;
  
  TextureResource resource;
  
  /// Indicates that this should be loaded by the Renderer and is used by the GState
  bool used;
  
  /// OpenCL texture name from glGenTexture
  uint64_t gl_texture;

  /// OpenGL texture type; CUBE_MAP, CUBE_MAP_ARRAY, TEXTURE_2D, etc
  uint64_t gl_texture_type;

  /// OpenGL texture location in shader from glUniformLocation
  uint64_t gl_texture_location;
  
  /// Layer index for the textures sampler type
  uint32_t layer_idx;
  
  /// Function of the texture
  enum class Type: int8_t {
      Diffuse, Specular
  };
  
  Texture::Type type;
  
  /// Texture id
  ID id;
};

#endif // MEINEKRAFT_TEXTURE_H
