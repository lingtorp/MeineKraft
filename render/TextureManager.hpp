#ifndef MEINEKRAFT_TEXTUREMANAGER_HPP
#define MEINEKRAFT_TEXTUREMANAGER_HPP

#include "texture.h"

/**
 * Loads, stores, Textures used by the Renderer
 */
class TextureManager {
private:
public:
    Texture load_texture_from(std::string filepath);

    bool is_texture_loaded(std::string filepath);
};

#endif //MEINEKRAFT_TEXTUREMANAGER_HPP
