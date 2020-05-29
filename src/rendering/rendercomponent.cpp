#include <SDL_opengl.h> // TODO: Remove.

#include "rendercomponent.hpp"
#include "render_system.hpp"
#include "../nodes/entity.hpp"
#include "meshmanager.hpp"

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
      case Texture::Type::TangentNormal:
        normal_texture.data = Texture::load_textures(resource);
        normal_texture.gl_texture_target = GL_TEXTURE_2D;
        normal_texture.id = resource.to_hash();
        break;
       default:
        Log::warn("Tried to load unsupported texture: " + texture_file);
    }
  }
}

std::vector<RenderComponent> 
RenderComponent::load_scene_models(const std::string& directory, const std::string& file) {
  std::vector<RenderComponent> render_components;

  std::vector<std::vector<std::pair<Texture::Type, std::string>>> texture_infos;
  std::vector<ID> mesh_ids;
  std::tie(mesh_ids, texture_infos) = MeshManager::load_meshes(directory, file);
    
  for (size_t i = 0; i < mesh_ids.size(); i++) {
    RenderComponent render_component;
    render_component.mesh_id = mesh_ids[i];

    // Load textures
    for (const auto& pair : texture_infos[i]) {
      auto texture_type = pair.first;
      auto texture_file = pair.second;
      const auto resource = TextureResource{ texture_file };
      switch (texture_type) {
      case Texture::Type::Diffuse:
        render_component.diffuse_texture.data = Texture::load_textures(resource);
        render_component.diffuse_texture.gl_texture_target = GL_TEXTURE_2D_ARRAY; // FIXME: Assumes texture format
        render_component.diffuse_texture.id = resource.to_hash();
        break;
      case Texture::Type::MetallicRoughness:
        render_component.metallic_roughness_texture.data = Texture::load_textures(resource);
        render_component.metallic_roughness_texture.gl_texture_target = GL_TEXTURE_2D; // FIXME: Assumes texture format
        render_component.metallic_roughness_texture.id = resource.to_hash();
        break;
      case Texture::Type::AmbientOcclusion:
        render_component.ambient_occlusion_texture.data = Texture::load_textures(resource);
        render_component.ambient_occlusion_texture.gl_texture_target = GL_TEXTURE_2D;
        render_component.ambient_occlusion_texture.id = resource.to_hash();
        break;
      case Texture::Type::Emissive:
        render_component.emissive_texture.data = Texture::load_textures(resource);
        render_component.emissive_texture.gl_texture_target = GL_TEXTURE_2D;
        render_component.emissive_texture.id = resource.to_hash();
        break;
      case Texture::Type::TangentNormal:
        render_component.normal_texture.data = Texture::load_textures(resource);
        render_component.normal_texture.gl_texture_target = GL_TEXTURE_2D;
        render_component.normal_texture.id = resource.to_hash();
        break;
      default:
        Log::warn("Tried to load unsupported texture: " + texture_file);
      }
    }

    render_components.push_back(render_component);
  }

  return render_components;
}

void RenderComponent::set_cube_map_texture(const std::array<std::string, 6>& faces) {
  // FIXME: Assumes the diffuse texture?
  std::vector<std::string> rsrcs(faces.size());
  std::copy(faces.begin(), faces.end(), rsrcs.begin());
  const auto resource = TextureResource{rsrcs};
  diffuse_texture.id = resource.to_hash();
  diffuse_texture.data = TextureManager::textures[diffuse_texture.id];
  diffuse_texture.gl_texture_target = GL_TEXTURE_CUBE_MAP_ARRAY;
  if (!diffuse_texture.data.pixels) {
    TextureManager::textures[diffuse_texture.id] = Texture::load_textures(resource);
    Log::info("Loading new cube map textures with id:" + std::to_string(diffuse_texture.id));
  }
}
