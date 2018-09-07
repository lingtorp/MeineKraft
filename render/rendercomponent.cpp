#include <SDL_opengl.h> // TODO: Remove.

#include "rendercomponent.h"
#include "render.h"
#include "../nodes/entity.h"
#include "meshmanager.h"

#ifdef _WIN32
#include <SDL_log.h>
#else
#include <sdl2/SDL_log.h>
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
        exit(1);
        break;
      default:
        exit(1);
    }
  }
};

void RenderComponent::update() {
  // FIXME: Virtual function in order to copy?
  graphics_state.scale = entity->scale;
  graphics_state.position = entity->position;
}

void RenderComponent::set_mesh(MeshPrimitive primitive) {
  // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
  // FIXME: How to get mesh ids for primitive meshes?
  switch (primitive) {
    case MeshPrimitive::Cube:
      graphics_state.mesh_id = 0;
      graphics_state.position = Vec3<float>();
      std::cerr << "Loading cube primitve";
      break;
    default:
      std::cerr << "Loading unknown primitve";
      break;
  }
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
