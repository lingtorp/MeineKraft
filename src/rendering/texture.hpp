#pragma once
#ifndef MEINEKRAFT_TEXTURE_HPP
#define MEINEKRAFT_TEXTURE_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

#include <SDL2/SDL_image.h>

#include "../util/logging.hpp"

/// Opaque ID type used to reference resources throughout the engine
typedef uint64_t ID; // FIXME: Wtf, double defined?

struct RawTexture {
  uint8_t* pixels = nullptr;
  uint8_t  bytes_per_pixel = 0;
  uint32_t size   = 0; // Byte size per face
  uint32_t width  = 0; // Measured in pixels
  uint32_t height = 0; 
  uint32_t faces  = 0; // Number of faces, used for cube maps
  RawTexture() = default;
};

struct TextureResource {
  std::vector<std::string> files;
  
  explicit TextureResource(const std::string& file): files{file} {};
  explicit TextureResource(const std::vector<std::string>& files): files{files} {};
  
  uint64_t to_hash() const {
    uint64_t hash = 0;
    for (const auto& file : files) {
      hash += std::hash<std::string>{}(file);
    }
    return hash;
  }
};

enum class ImageFormat {
  PPM,
  PNG
};

enum class TextureFormat {
  RGB32F,
  R32F
};

struct Texture {
  static RawTexture load_textures(const TextureResource& resource);
  
  /// Texture id
  ID id = 0;
  
  RawTexture data;

  /// OpenGL texture target; CUBE_MAP, CUBE_MAP_ARRAY, TEXTURE_2D, etc
  uint32_t gl_texture_target = 0;

  TextureFormat format; // FIXME: Use!

  enum class Type: uint8_t {
    Diffuse,              // A.k.a albedo/base_color
    MetallicRoughness,    // glTF 2.0 material model
    AmbientOcclusion,     // 
    Emissive,             // Emission texture
    TangentNormal         // Tangent space normal map/texture
  };
};

#endif // MEINEKRAFT_TEXTURE_HPP
