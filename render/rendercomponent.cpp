#include <SDL_opengl.h> // TODO: Remove.

#include "rendercomponent.h"
#include "render.h"
#include "../nodes/entity.h"
#include "meshmanager.h"

void RenderComponent::set_mesh(const std::string& directory, const std::string& file) {
  // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
  std::vector<std::pair<Texture::Type, std::string>> texture_info;
  std::tie(mesh_id, texture_info) = MeshManager::load_mesh(directory, file);
  for (const auto& pair : texture_info) {
    auto texture_type = pair.first;
    auto texture_file = pair.second;
    const auto resource = TextureResource{texture_file};
    switch (texture_type) {
      case Texture::Type::Diffuse:
        diffuse_texture.data = Texture::load_textures(resource);
        if (diffuse_texture.data.pixels) {
          diffuse_texture.gl_texture_target = GL_TEXTURE_2D_ARRAY; // FIXME: Assumes texture format
          diffuse_texture.used = true;
          diffuse_texture.id = resource.to_hash();
        }
        break;
      case Texture::Type::MetallicRoughness:
        metallic_roughness_texture.data = Texture::load_textures(resource);
        if (metallic_roughness_texture.data.pixels) {
          metallic_roughness_texture.gl_texture_target = GL_TEXTURE_2D; // FIXME: Assumes texture format
          metallic_roughness_texture.used = true;
          metallic_roughness_texture.id = resource.to_hash();
        }
        break;
      case Texture::Type::AmbientOcclusion:
        ambient_occlusion_texture.data = Texture::load_textures(resource);
        if (ambient_occlusion_texture.data.pixels) {
          ambient_occlusion_texture.gl_texture_target = GL_TEXTURE_2D;
          ambient_occlusion_texture.used = true;
          ambient_occlusion_texture.id = resource.to_hash();
        }
        break;   
      case Texture::Type::Emissive:
        emissive_texture.data = Texture::load_textures(resource);
        if (emissive_texture.data.pixels) {
          emissive_texture.gl_texture_target = GL_TEXTURE_2D;
          emissive_texture.used = true;
          emissive_texture.id = resource.to_hash();
        }
        break;
       default:
        Log::warn("Tried to load unsupported texture: " + texture_file);
    }
  }
};

void RenderComponent::set_cube_map_texture(const std::vector<std::string>& faces) {
  // FIXME: Assumes the diffuse texture?
  auto resource = TextureResource{faces};
  diffuse_texture.data = Texture::load_textures(resource);
  if (diffuse_texture.data.pixels) {
    diffuse_texture.gl_texture_target = GL_TEXTURE_CUBE_MAP_ARRAY;
    diffuse_texture.used = true;
    diffuse_texture.id = resource.to_hash();
  } else {
    Log::warn("Failed to load cube map textures");
  }
}
