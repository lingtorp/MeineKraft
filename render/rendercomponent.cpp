#include <SDL_opengl.h> // TODO: Remove.

#include "rendercomponent.h"
#include "render.h"
#include "../nodes/entity.h"
#include "meshmanager.h"

#ifdef _WIN32
#include <SDL_log.h>
#else
#include <SDL2/SDL_log.h>
#endif 

RenderComponent::RenderComponent(Entity* entity): entity(entity), graphics_state{} {}

void RenderComponent::set_mesh(const std::string& directory, const std::string& file) {
  // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
  ID mesh_id;
  std::vector<std::pair<Texture::Type, std::string>> texture_info;
  std::tie(mesh_id, texture_info) = MeshManager::load_mesh(directory, file);
  graphics_state.mesh_id = mesh_id;
  for (const auto& pair : texture_info) {
    auto texture_type = pair.first;
    auto texture_file = pair.second;
    const auto resource = TextureResource{texture_file};
    switch (texture_type) {
      case Texture::Type::Diffuse:
        graphics_state.diffuse_texture.data = Texture::load_textures(resource);
        if (graphics_state.diffuse_texture.data.pixels) {
          graphics_state.diffuse_texture.gl_texture_type = GL_TEXTURE_2D_ARRAY; // FIXME: Assumes texture format
          graphics_state.diffuse_texture.used = true;
          graphics_state.diffuse_texture.id = resource.to_hash();
        }
        break;
      case Texture::Type::Specular:
        graphics_state.specular_texture.data = Texture::load_textures(resource);
        if (graphics_state.specular_texture.data.pixels) {
          graphics_state.specular_texture.gl_texture_type = GL_TEXTURE_2D_ARRAY; // FIXME: Assumes texture format
          graphics_state.specular_texture.used = true;
          graphics_state.specular_texture.id = resource.to_hash();
        }
        break;
      default:
        exit(1);
    }
  }
};

void RenderComponent::update() {
  graphics_state.scale = entity->scale;
  graphics_state.position = entity->position;
}

void RenderComponent::set_mesh(MeshPrimitive primitive) {
  // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
  graphics_state.mesh_id = MeshManager::mesh_id_from_primitive(primitive);
}

void RenderComponent::set_cube_map_texture(const std::vector<std::string>& faces) {
  // FIXME: Assumes the diffuse texture?
  auto resource = TextureResource{faces};
  graphics_state.diffuse_texture.data = Texture::load_textures(resource);
  if (graphics_state.diffuse_texture.data.pixels) {
    graphics_state.diffuse_texture.gl_texture_type = GL_TEXTURE_CUBE_MAP_ARRAY;
    graphics_state.diffuse_texture.used = true;
    graphics_state.diffuse_texture.id = resource.to_hash();
  }
}

/// RenderComponents are not supposed to be modified and only re-created
void RenderComponent::did_attach_to_entity(Entity* entity) {
  Renderer::instance().add_to_batch(this);
}
