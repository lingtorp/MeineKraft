#include <SDL2/SDL_log.h>
#include <SDL2/SDL_image.h>
#include "texturemanager.h"

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
