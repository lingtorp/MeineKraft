#include <SDL2/SDL_log.h>
#include <SDL2/SDL_image.h>
#include "texturemanager.h"

std::vector<std::pair<Texture::Type, Texture>>
TextureManager::load_textures(std::vector<std::pair<Texture::Type, std::string>> texture_info) {
    std::vector<std::pair<Texture::Type, Texture>> textures;
  /* // FIXME: ...
    for (auto &info : texture_info) {
        auto texture_type     = info.first;
        auto texture_filepath = info.second;
        Texture texture;
        texture.load(texture_filepath);
        if (texture.loaded_successfully) {
            textures.emplace_back(texture_type, texture);
        } else {
            SDL_Log("TextureManager: Could not load texture from %s", texture_filepath.c_str());
        }
    }
    */
    return textures;
}

RawTexture TextureManager::lookup(ID id) {
  for (auto& texture : cache) {
    if (texture.id == id) {
      return texture;
    }
  }
  return {0, nullptr};
}

ID TextureManager::insert(RawTexture texture) {
  cache.push_back(texture);
  return 0;
}
