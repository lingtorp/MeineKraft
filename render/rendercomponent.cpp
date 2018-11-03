#include <SDL_opengl.h> // TODO: Remove.

#include "rendercomponent.h"
#include "render.h"
#include "../nodes/entity.h"
#include "meshmanager.h"

namespace TextureManager {
  static std::unordered_map<ID, RawTexture> textures{};
};

void RenderComponent::set_mesh(const std::string& directory, const std::string& file) {
  std::vector<std::pair<Texture::Type, std::string>> texture_info;
  std::tie(mesh_id, texture_info) = MeshManager::load_mesh(directory, file);
  for (const auto& pair : texture_info) {
    auto texture_type = pair.first;
    auto texture_file = pair.second;
    const auto resource = TextureResource{texture_file};
    switch (texture_type) {
      case Texture::Type::Diffuse:
        diffuse_texture.data = Texture::load_textures(resource);
        diffuse_texture.gl_texture_target = GL_TEXTURE_2D_ARRAY; // FIXME: Assumes texture format
        diffuse_texture.id = resource.to_hash();
        break;
      case Texture::Type::MetallicRoughness:
        metallic_roughness_texture.data = Texture::load_textures(resource);
        metallic_roughness_texture.gl_texture_target = GL_TEXTURE_2D; // FIXME: Assumes texture format
        metallic_roughness_texture.id = resource.to_hash();
        break;
      case Texture::Type::AmbientOcclusion:
        ambient_occlusion_texture.data = Texture::load_textures(resource);
        ambient_occlusion_texture.gl_texture_target = GL_TEXTURE_2D;
        ambient_occlusion_texture.id = resource.to_hash();
        break;   
      case Texture::Type::Emissive:
        emissive_texture.data = Texture::load_textures(resource);
        emissive_texture.gl_texture_target = GL_TEXTURE_2D;
        emissive_texture.id = resource.to_hash();
        break;
       default:
        Log::warn("Tried to load unsupported texture: " + texture_file);
    }
  }
};

void RenderComponent::set_cube_map_texture(const std::vector<std::string>& faces) {
  // FIXME: Assumes the diffuse texture?
  const auto resource = TextureResource{faces};
  diffuse_texture.id = resource.to_hash();
  diffuse_texture.data = TextureManager::textures[diffuse_texture.id];
  diffuse_texture.gl_texture_target = GL_TEXTURE_CUBE_MAP_ARRAY;
  if (!diffuse_texture.data.pixels) {
    TextureManager::textures[diffuse_texture.id] = Texture::load_textures(resource);
    Log::info("Loading new cube map textures with id:" + std::to_string(diffuse_texture.id));
  }
}
