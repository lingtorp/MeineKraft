#ifndef MEINEKRAFT_TEXTUREMANAGER_HPP
#define MEINEKRAFT_TEXTUREMANAGER_HPP

#include "texture.h"
#include "primitives.h"

/**
 * Loads, stores, Textures used by the Renderer
 */
// TODO: Need to change so that not every texture is loaded multiple times, TextureManager should coordinate it
// TODO: Cache textures in TextureManager
class TextureManager {
public:
    TextureManager();

    std::vector<std::pair<Texture::Type, Texture>>
    load_textures(std::vector<std::pair<Texture::Type, std::string>> texture_info);

private:
};

#endif //MEINEKRAFT_TEXTUREMANAGER_HPP
