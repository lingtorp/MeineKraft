#ifndef MEINEKRAFT_TEXTURE_H
#define MEINEKRAFT_TEXTURE_H

#include <cstdint>
#include <string>
#include <vector>
#include <SDL2/SDL_image.h>

/// Opaque ID type used to reference resources throughout the engine
typedef uint64_t ID;

struct RawTexture {
  ID id;
  uint8_t* pixels = nullptr;
  size_t size;
  uint32_t width;
  uint32_t height;
  RawTexture() = default;
  RawTexture(ID id, uint8_t* data): id(id), pixels(data), width(0), height(0), size(0) {}
};

struct TextureResource {
  TextureResource() = default;
  std::vector<std::string> files;
  
  explicit TextureResource(std::string file): files{file} {};
  explicit TextureResource(std::vector<std::string> files): files{files} {};
  
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
  
  RawTexture load_textures() {
    RawTexture texture{0, nullptr};
    auto& files = resource.files;
    
    if (files.empty()) { return texture; }
    
    auto& file = files.front();
    SDL_Surface* image = IMG_Load(file.c_str()); // FIXME: NULL? Handle error with IMG_GetError
    texture.width  = static_cast<uint32_t>(image->w);
    texture.height = static_cast<uint32_t>(image->h);
    auto bytes_per_pixel = image->format->BytesPerPixel;
    SDL_FreeSurface(image); // FIXME: Wasteful
    
    // Assumes that the files are the same size, in right order, same encoding, etc
    texture.size = bytes_per_pixel * texture.width * texture.height;
    texture.pixels = static_cast<uint8_t*>(std::calloc(1, texture.size * files.size()));
    // Load all the files into a linear memory region
    for (size_t i = 0; i < files.size(); i++) {
      image = IMG_Load(files[i].c_str());
      // Convert it to OpenGL friendly format
      auto desired_img_format = bytes_per_pixel == 3 ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA32;
      SDL_Surface* conv = SDL_ConvertSurfaceFormat(image, desired_img_format, 0);
      std::memcpy(texture.pixels + texture.size * i, conv->pixels, texture.size);
      SDL_FreeSurface(image);
    }
    
    return texture;
  }
  
  /// Texture id
  ID id;
  
  RawTexture data;
  TextureResource resource;
  
  /// Indicates that this should be loaded by the Renderer and is used by the GState
  bool used;

  /// OpenGL texture type; CUBE_MAP, CUBE_MAP_ARRAY, TEXTURE_2D, etc
  uint32_t gl_texture_type;
  
  /// Layer index for the textures sampler type
  uint32_t layer_idx;
  
  /// Function of the texture
  enum class Type: int8_t {
      Diffuse, Specular
  };
  
  Texture::Type type;
};

#endif // MEINEKRAFT_TEXTURE_H
