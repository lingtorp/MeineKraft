#include "texture.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

#include <SDL2/SDL_image.h>

#include "../util/logging.hpp"

RawTexture Texture::load_textures(const TextureResource& resource) {
  RawTexture texture{};

  if (resource.files.empty()) { return texture; }

  // Load all the files into a linear memory region
  // NOTE: Assumes that the files are the same size, in right order, same encoding, etc
  for (size_t i = 0; i < resource.files.size(); i++) {
    SDL_Surface* image = IMG_Load(resource.files[i].c_str());
    if (!image) {
      Log::error("Could not load texture: " + std::string(IMG_GetError()));
      continue;
    }
    texture.width = static_cast<uint32_t>(image->w);
    texture.height = static_cast<uint32_t>(image->h);
    texture.bytes_per_pixel = image->format->BytesPerPixel;
    texture.size = texture.bytes_per_pixel * texture.width * texture.height;
    if (i == 0) { // Allocate all the memory on the first iteration
      texture.pixels = static_cast<uint8_t*>(std::calloc(1, texture.size * resource.files.size()));
    }

    // Convert it to OpenGL friendly format if needed
    const auto desired_img_format = texture.bytes_per_pixel == 3 ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA32;
    if (image->format->format == desired_img_format) {
      std::memcpy(texture.pixels + texture.size * i, image->pixels, texture.size);
    } else {
      SDL_Surface* conv = SDL_ConvertSurfaceFormat(image, desired_img_format, 0);
      if (conv) {
        std::memcpy(texture.pixels + texture.size * i, conv->pixels, texture.size);
        SDL_FreeSurface(conv);
      } else {
        Log::error("RawTexture loading failed: " + std::string(SDL_GetError()));
      }
    }
    SDL_FreeSurface(image);
    texture.faces++;
  }

  return texture;
}
