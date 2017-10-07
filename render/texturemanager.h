#ifndef MEINEKRAFT_TEXTUREMANAGER_HPP
#define MEINEKRAFT_TEXTUREMANAGER_HPP

#include "texture.h"
#include "primitives.h"

struct RawTexture {
  ID id;
  uint8_t* data = nullptr;
  size_t size;
  uint32_t width;
  uint32_t height;
  RawTexture(ID id, uint8_t* data): id(id), data(data), width(0), height(0), size(0) {}
  // Texture encoding (GL_RGB, etc)
};

class TextureManager {
public:
  TextureManager() = default;
  
  RawTexture lookup(ID id);
  ID insert(RawTexture texture);

private:
  std::vector<RawTexture> cache;
};

#endif //MEINEKRAFT_TEXTUREMANAGER_HPP
