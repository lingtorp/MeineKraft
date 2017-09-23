#include <SDL2/SDL_log.h>
#include "texturemanager.h"

TextureManager::TextureManager() {}

std::vector<std::pair<Texture::Type, Texture>>
TextureManager::load_textures(std::vector<std::pair<Texture::Type, std::string>> texture_info) {
    std::vector<std::pair<Texture::Type, Texture>> textures;
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
    return textures;
}
